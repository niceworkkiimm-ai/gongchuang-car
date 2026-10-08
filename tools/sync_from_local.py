"""Back up the configured project and this conversation; never edit source trees."""
from pathlib import Path
from datetime import datetime, timezone, timedelta
import collections
import fnmatch
import hashlib
import json
import re
import shutil

ROOT = Path(__file__).resolve().parents[1]
LOCAL = ROOT / '.local'
CONFIG = LOCAL / 'sources.json'
STATE = LOCAL / 'sync-state.json'
SKIP_DIRS = {'.git', '.local', '__pycache__', 'objects', 'listings', 'node_modules', '.venv'}
SKIP_NAMES = ('.env', '.env.*', '*.pem', '*.key', '*.pyc', '*.uvguix.*', 'Thumbs.db')


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def destination(relative):
    path = (ROOT / relative).resolve()
    try:
        path.relative_to(ROOT)
    except ValueError:
        raise RuntimeError('Destination escapes repository: ' + str(relative))
    if path == ROOT:
        raise RuntimeError('Destination escapes repository: ' + str(relative))
    if path.parts[len(ROOT.parts)].lower() in {'.git', '.local'}:
        raise RuntimeError('Managed destination is reserved: ' + str(relative))
    return path


def allowed(relative):
    return not any(p.lower() in SKIP_DIRS for p in relative.parts) and not any(
        fnmatch.fnmatch(relative.name, pattern) for pattern in SKIP_NAMES)


def conversation(config):
    thread = config['thread_id']
    files = sorted(Path(config['sessions_root']).rglob('*' + thread + '*.jsonl'))
    if not files:
        raise RuntimeError('Conversation source logs are unavailable; retain existing export.')
    messages, seen, images = [], set(), {}
    for path in files:
        for line in path.open(encoding='utf-8'):
            try:
                event = json.loads(line)
            except json.JSONDecodeError:
                continue  # The active session may still be writing its last line.
            payload = event.get('payload', {})
            item = payload.get('item', {})
            if event.get('type') != 'event_msg' or payload.get('type') != 'item_completed':
                continue
            if payload.get('thread_id') != thread:
                continue
            role = {'UserMessage': '用户', 'AgentMessage': '助手'}.get(item.get('type'))
            if not role:
                continue  # Do not export reasoning, tool output, credentials or system instructions.
            parts = item.get('content', [])
            text = '\n'.join(x.get('text', '') for x in parts if isinstance(x, dict)).strip()
            attachments = []
            for part in parts:
                if part.get('type') == 'local_image':
                    image = Path(part['path'])
                    if image.is_file():
                        name = digest(image)[:12] + image.suffix.lower()
                        relative = 'docs/conversation/images/' + name
                        images[relative] = image
                        attachments.append('images/' + name)
            if not text and not attachments:
                continue
            key = item.get('id') or (event.get('timestamp'), role, text)
            if key in seen:
                continue
            seen.add(key)
            messages.append({'timestamp': event.get('timestamp', ''), 'role': role,
                             'text': text, 'images': attachments})
    messages.sort(key=lambda x: x['timestamp'])
    grouped = collections.defaultdict(list)
    for message in messages:
        stamp = datetime.fromisoformat(message['timestamp'].replace('Z', '+00:00'))
        local = stamp.astimezone(timezone(timedelta(hours=8)))
        grouped[local.strftime('%Y-%m')].append((local, message))
    generated = {}
    for month, entries in sorted(grouped.items()):
        out = ['# 工创项目对话记录 · ' + month,
               '', '从本项目本地会话日志导出，仅包含可见的用户和助手消息。',
               '历史建议可能已被后续修改，以当前源码和较新的记录为准。', '']
        for stamp, msg in entries:
            out.extend(['## ' + stamp.strftime('%Y-%m-%d %H:%M:%S') + ' · ' + msg['role'],
                        '', msg['text'], ''])
            out.extend('![对话附件](' + name + ')\n' for name in msg['images'])
        generated['docs/conversation/' + month + '.md'] = '\n'.join(out).encode('utf-8')
    generated['docs/conversation/index.json'] = json.dumps({
        'thread_id': thread, 'source_session_files': len(files),
        'messages': len(messages), 'last_message_time': messages[-1]['timestamp'] if messages else None,
        'months': sorted(grouped), 'available_images': len(images),
        'note': 'Only visible UserMessage/AgentMessage items; unavailable attachments are not reconstructed.'
    }, ensure_ascii=False, indent=2).encode('utf-8') + b'\n'
    return generated, images


def main():
    config = json.loads(CONFIG.read_text(encoding='utf-8'))
    previous = json.loads(STATE.read_text(encoding='utf-8')) if STATE.exists() else {}
    plan = {}
    catalog = []
    for source in config['sources']:
        origin = Path(source['path'])
        if not origin.exists():
            raise RuntimeError('Configured source is missing; sync stopped: ' + str(origin))
        entries = [origin] if origin.is_file() else sorted(p for p in origin.rglob('*') if p.is_file())
        copied = 0
        for path in entries:
            relative = Path(path.name) if origin.is_file() else path.relative_to(origin)
            if not allowed(relative):
                continue
            key = source['destination'] if origin.is_file() else str(Path(source['destination']) / relative)
            key = key.replace('\\', '/')
            if key in plan:
                raise RuntimeError('Duplicate destination: ' + key)
            destination(key)
            plan[key] = path
            copied += 1
        catalog.append({'label': source['label'], 'destination': source['destination'], 'files': copied})
    generated, images = conversation(config)
    plan.update(images)
    generated['docs/source-catalog.json'] = json.dumps(catalog, ensure_ascii=False, indent=2).encode('utf-8') + b'\n'
    hashes = {key: digest(path) for key, path in plan.items()}
    hashes.update({key: hashlib.sha256(data).hexdigest() for key, data in generated.items()})
    # Detect edits in the backup before any copy/delete; never silently discard them.
    for key, value in hashes.items():
        dest = destination(key)
        if dest.exists():
            actual = digest(dest)
            if actual != value and actual != previous.get(key):
                raise RuntimeError('Backup has a local edit; resolve before syncing: ' + key)
    removed = set(previous) - set(hashes)
    for key in removed:
        dest = destination(key)
        if dest.exists() and digest(dest) != previous[key]:
            raise RuntimeError('Removed source has an edited backup; retaining it: ' + key)
    for key, path in plan.items():
        dest = destination(key)
        if not dest.exists() or digest(dest) != hashes[key]:
            dest.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(path, dest)
    for key, data in generated.items():
        dest = destination(key)
        if not dest.exists() or dest.read_bytes() != data:
            dest.parent.mkdir(parents=True, exist_ok=True)
            dest.write_bytes(data)
    for key in removed:
        dest = destination(key)  # Revalidate before deleting a previously managed backup file.
        if dest.exists():
            dest.unlink()
    for key, expected in hashes.items():
        if digest(destination(key)) != expected:
            raise RuntimeError('Copy verification failed: ' + key)
    STATE.write_text(json.dumps(hashes, ensure_ascii=False, indent=2), encoding='utf-8')
    print('Verified backup files:', len(hashes))
    print('Conversation messages:', json.loads(generated['docs/conversation/index.json'])['messages'])
    print('Source files were not modified.')


if __name__ == '__main__':
    main()
