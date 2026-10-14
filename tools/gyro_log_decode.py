"""Decode the car's gyro_log RAM snapshot (Keil Intel HEX or raw binary)."""
import argparse
import csv
import json
import math
import re
import struct
from pathlib import Path

MAGIC = 0x474C5947
HEADER = struct.Struct('<16I')
RECORD = struct.Struct('<IfffIIhhHBB')
EVENTS = {
    1: '启动', 2: '底盘动作开始', 3: '转向速度指令发送后',
    4: '进入角度容差', 5: '停车指令发送后', 6: '底盘函数返回',
    7: '调正返回后至少50ms', 8: '调正返回后至少200ms',
    9: '调正返回后至少500ms', 10: '最终回程返回',
    11: '日志冻结', 100: '角度采样',
}


def intel_hex(text):
    memory, base, eof = {}, 0, False
    for number, line in enumerate(text.splitlines(), 1):
        line = line.strip()
        if not line:
            continue
        if eof or not line.startswith(':'):
            raise ValueError(f'第{number}行不是有效的HEX记录')
        data = bytes.fromhex(line[1:])
        if len(data) < 5 or len(data) != data[0] + 5 or sum(data) & 255:
            raise ValueError(f'第{number}行长度或校验错误')
        size, address, kind = data[0], int.from_bytes(data[1:3], 'big'), data[3]
        payload = data[4:-1]
        if kind == 0:
            for offset, value in enumerate(payload):
                key = base + address + offset
                if key in memory and memory[key] != value:
                    raise ValueError('HEX含相互冲突的重叠数据')
                memory[key] = value
        elif kind == 1 and size == 0:
            eof = True
        elif kind == 2 and size == 2:
            base = int.from_bytes(payload, 'big') << 4
        elif kind == 4 and size == 2:
            base = int.from_bytes(payload, 'big') << 16
        elif kind in (3, 5) and size == 4:
            pass
        else:
            raise ValueError(f'不支持或无效的HEX记录类型{kind}')
    if not eof:
        raise ValueError('HEX缺少文件结束记录')
    return memory


def validate_header(values):
    magic, version, hsize, rsize, ecap, scap, ecnt, scnt, snext = values[:9]
    if magic != MAGIC or version != 1 or hsize != 64 or rsize != 32:
        raise ValueError('日志格式不匹配，请使用对应版本的导出工具')
    if not (0 < ecap <= 65536 and 0 < scap <= 65536):
        raise ValueError('日志容量无效')
    if ecnt > ecap or scnt > scap or snext >= scap:
        raise ValueError('日志计数无效，可能读取地址错误或读取时程序仍在写入')
    if scnt < scap and snext != scnt:
        raise ValueError('采样指针与计数不一致')
    if values[11] not in (0, 1) or not values[12]:
        raise ValueError('日志状态或系统时钟无效')
    return hsize + rsize * (ecap + scap)


def read_snapshot(path):
    raw = Path(path).read_bytes()
    matches = []
    if raw.lstrip().startswith(b':'):
        memory = intel_hex(raw.decode('ascii'))
        for address in sorted(memory):
            if memory[address] != 0x47:
                continue
            try:
                header = bytes(memory[address + i] for i in range(64))
                values = HEADER.unpack(header)
                if values[0] != MAGIC:
                    continue
                size = validate_header(values)
                blob = bytes(memory[address + i] for i in range(size))
            except (KeyError, ValueError):
                continue
            matches.append((address, values, blob))
    else:
        marker = struct.pack('<I', MAGIC)
        pos = raw.find(marker)
        while pos >= 0:
            try:
                values = HEADER.unpack_from(raw, pos)
                size = validate_header(values)
                if pos + size <= len(raw):
                    matches.append((pos, values, raw[pos:pos + size]))
            except (struct.error, ValueError):
                pass
            pos = raw.find(marker, pos + 1)
    if len(matches) != 1:
        raise ValueError('未找到唯一完整的gyro_log；请导出日志整个地址范围，不是程序HEX')
    return matches[0]


def records(values, blob):
    ecap, scap, ecnt, scnt, snext = values[4:9]
    result = []
    for bank in ('事件', '采样'):
        indexes = range(ecnt) if bank == '事件' else (
            [(snext + i) % scap for i in range(scnt)] if scnt == scap else range(scnt))
        base = 64 if bank == '事件' else 64 + ecap * 32
        epoch, previous = 0, None
        group = []
        for i in indexes:
            fields = RECORD.unpack_from(blob, base + i * 32)
            if not all(math.isfinite(x) for x in fields[1:4]):
                raise ValueError('日志中存在非有限角度，需检查导出是否完整')
            tick = fields[0]
            if previous is not None and tick < previous:
                epoch += 1 << 32
            previous = tick
            group.append({'bank': bank, 'index': i, 'time': tick + epoch,
                          'fields': fields})
        if bank == '采样' and group and result:
            # Samples retain only the newest part, while events start at boot.
            shift = round((result[-1]['time'] - group[-1]['time']) / (1 << 32))
            if shift > 0:
                for item in group:
                    item['time'] += shift * (1 << 32)
        result.extend(group)
    return sorted(result, key=lambda r: (r['time'], r['bank'] == '采样', r['index']))


def wrapped_error(target, angle):
    return (target - angle + 180) % 360 - 180


def write_csv(path, rows, fields):
    with Path(path).open('w', newline='', encoding='utf-8-sig') as f:
        writer = csv.writer(f)
        writer.writerow(fields)
        writer.writerows(rows)


def export(path, output):
    address, values, blob = read_snapshot(path)
    entries = records(values, blob)
    rows, summaries, heading = [], [], None
    for item in entries:
        t, yaw, control, rate, frames, last_yaw, target, mode, event, motor, status = item['fields']
        direction, active = status & 1, bool(status & 128)
        age = ((t - last_yaw) & 0xFFFFFFFF) / 1000 if frames else ''
        rows.append([round(item['time'] / 1000, 3), item['bank'],
                     EVENTS.get(event, f'未知事件{event}'), mode, int(active), target,
                     f'{yaw:.6f}', f'{control:.6f}', f'{rate:.6f}', frames, age,
                     motor or '', direction if motor else '',
                     f'{wrapped_error(target, yaw):.6f}'])
        if item['bank'] != '事件':
            continue
        if event == 2 and mode == 0:
            heading = {'start_ms': item['time'] / 1000, 'target': target,
                       'start_yaw': yaw, 'directions': set(), 'stop_motors': [],
                       'window': '', 'end_yaw': '', 'end_error': '',
                       'post50': '', 'post200': '', 'post500': '', 'end_ms': ''}
            summaries.append(heading)
        if heading is not None:
            if event == 3:
                heading['directions'].add(direction)
            elif event == 4:
                heading['window'] = yaw
            elif event == 5:
                heading['stop_motors'].append(motor)
            elif event == 6 and mode == 0:
                heading['end_yaw'] = yaw
                heading['end_error'] = wrapped_error(target, yaw)
                heading['end_ms'] = item['time'] / 1000
            elif event in (7, 8, 9):
                heading[{7: 'post50', 8: 'post200', 9: 'post500'}[event]] = yaw
    out = Path(output)
    out.parent.mkdir(parents=True, exist_ok=True)
    write_csv(out, rows, ['时间_ms', '记录区', '事件', '底盘模式', '底盘函数执行中', '最近调正目标_deg',
              '陀螺仪角度_deg', '控制角度_deg', '角速度_deg_s', '有效角度帧数',
              '最新角度帧龄_ms', '电机编号', '方向', '目标减陀螺仪角度_deg'])
    summary_path = out.with_name(out.stem + '_调正汇总.csv')
    write_csv(summary_path, [[i, h['start_ms'], h['target'], h['start_yaw'],
        h['window'], h['end_ms'], h['end_yaw'], h['end_error'], h['post50'],
        h['post200'], h['post500'], '是' if len(h['directions']) > 1 else '否',
        '/'.join(map(str, h['stop_motors']))] for i, h in enumerate(summaries, 1)],
        ['序号', '调正开始_ms', '目标_deg', '开始角度_deg', '进入容差角度_deg',
         '函数返回_ms', '函数返回角度_deg', '返回时误差_deg', '返回后50ms角度_deg',
         '返回后200ms角度_deg', '返回后500ms角度_deg', '是否发过两个方向', '停车电机顺序'])
    meta = dict(zip(['magic', 'version', 'header_size', 'record_size',
        'event_capacity', 'sample_capacity', 'event_count', 'sample_count',
        'sample_next', 'sample_total', 'events_dropped', 'frozen', 'clock_hz',
        'max_write_cycles', 'max_tick_cycles', 'flags'], values))
    meta.update(snapshot_address=hex(address),
        samples_overwritten=max(0, values[9] - values[7]),
        max_record_body_us=values[13] * 1e6 / values[12],
        max_tick_body_us=values[14] * 1e6 / values[12],
        note='停车记录为发送函数返回，不是到位或电机ACK；后续角度可能发生在下一动作期间。')
    out.with_suffix('.json').write_text(json.dumps(meta, ensure_ascii=False, indent=2), encoding='utf-8')
    print(f'已导出：{out}\n调正汇总：{summary_path}\n事件{values[6]}条，采样{values[7]}条，冻结={values[11]}')
    if values[10]:
        print(f'提示：事件区已满，后续{values[10]}条事件未保存。')
    if not values[11]:
        print('提示：快照未冻结，读取前请先冻结日志并等待当前记录结束。')
    return meta, entries


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('input', nargs='?', type=Path, help='运行后导出的gyro_log.hex或RAM.bin')
    p.add_argument('--output', type=Path)
    p.add_argument('--map', type=Path, help='从Keil映射文件生成当前日志地址的SAVE命令')
    p.add_argument('--save-command', type=Path)
    args = p.parse_args()
    try:
        if args.map:
            text = args.map.read_text(errors='replace')
            match = re.search(r'^\s*gyro_log\s+(0x[0-9a-fA-F]+)\s+Data\s+(\d+)', text, re.M)
            if not match:
                raise ValueError('map中未找到gyro_log（可能日志开关关闭）')
            start, size = int(match[1], 16), int(match[2])
            command = f'SAVE gyro_log.hex 0x{start:08X}, 0x{start + size - 1:08X}\n'
            if args.save_command:
                args.save_command.write_text(command, encoding='ascii')
            print(command.strip())
        if args.input:
            export(args.input, args.output or args.input.with_suffix('.csv'))
        if not args.input and not args.map:
            p.error('请指定运行后导出的日志，或通过--map生成导出命令')
    except (ValueError, OSError, UnicodeError, struct.error) as e:
        p.exit(1, f'无法导出：{e}\n')


if __name__ == '__main__':
    main()
