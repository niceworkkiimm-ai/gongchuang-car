# run_all.py
# 圆检测 + 颜色识别 + 数字识别（YOLOv5）+ 串口
# 三合一：YOLO 跳帧运行，避免 NPU 过载

from maix import app, camera, display, image, time, nn

from config import CAM_W, CAM_H, SERIAL
from detector import detect_all, reset_state as reset_ring_state
from color_detector import detect_colors, reset_cache
from serial_link import create_serial_link


# ---- AI 模型 ----
MODEL_PATH = "/root/models/model_298897.mud"

# YOLO 每 N 帧跑一次，黄圈中心跟随数字框；1 表示每帧跟手
YOLO_SKIP = 1

# 数字识别置信度，低于这个值不参与数字结果
DIGIT_CONF_TH = 0.48

# 圆检测每 N 帧跑一次，用于确认确实有靶纸圆，避免背景误识别
HOLE_SKIP = 48

# 颜色检测每 N 帧跑一次，内部会用 ROI 低成本跟踪
COLOR_SKIP = 1

# 圆心候选离数字中心超过这个距离时，认为圆检测抓偏了
RING_DIGIT_MAX_DIST = 85

# 黄圈只平滑半径，不延迟中心
RING_R_ALPHA = 0.50
RING_MISS_KEEP_FRAMES = 4

# ============================================================
# 有效标签集合（只有这些标签才被当成数字发送）
# 模型可能还会检出其他东西，不在这个集合里的忽略
# ============================================================
VALID_LABELS = {
    "zero", "one", "two", "three", "four",
    "five", "six", "seven", "eight", "nine",
}

# ============================================================
# 数字形状校验 —— 解决 1/3 → 2 的误识别
# 手写体 1 通常窄高（宽高比 < 0.55），2 和 3 更宽
# 如果模型说是 "two" 但框明显是窄的，很可能是 1；反之亦然
# ============================================================
LABEL_TO_DIGIT = {
    "zero": 0, "one": 1, "two": 2, "three": 3, "four": 4,
    "five": 5, "six": 6, "seven": 7, "eight": 8, "nine": 9,
}
DIGIT_TO_LABEL = {v: k for k, v in LABEL_TO_DIGIT.items()}

# 宽高比的典型区间（根据你的实际数据集统计后调整更准）
# 1 通常 w/h 最小；2/3 中等；0/8 较宽
ASPECT_RATIO_RANGES = {
    1: (0.20, 0.60),   # one: 窄
    2: (0.40, 0.95),   # two: 中等偏宽
    3: (0.40, 0.95),   # three: 中等偏宽
    7: (0.25, 0.65),   # seven: 偏窄
}


def correct_digit_by_shape(label, w, h, score):
    """
    根据检测框的宽高比对模型输出做二次校正。
    返回 (corrected_label, corrected_score)。
    """
    if w <= 0 or h <= 0:
        return label, score

    ar = w / h  # aspect ratio

    # 如果模型说是 two/three，但框很窄（ar < 0.38），大概率是 one
    if label in ("two", "three") and ar < 0.38:
        # one 的宽高比通常在 0.25~0.55
        return "one", score * 0.85  # 略微降置信度

    # 如果模型说是 two，但框偏窄 + 置信度不高，可能是 one 或 seven
    if label == "two" and ar < 0.45 and score < 0.75:
        return "one", score * 0.8

    return label, score


def digit_over_color(yolo_objs, color_targets):
    if not yolo_objs:
        return False

    valid_objs = [o for o in yolo_objs if o.get("label") in VALID_LABELS]
    if not valid_objs:
        return False

    obj = max(valid_objs, key=lambda o: o.get("score", 0))
    anchor_x = int(obj["x"] + obj["w"] / 2)
    anchor_y = int(obj["y"] + obj["h"] / 2)
    for color in color_targets or []:
        dx = anchor_x - color["cx"]
        dy = anchor_y - color["cy"]
        limit = max(18, int(color["r"] * 0.85))
        if dx * dx + dy * dy <= limit * limit:
            return True
    return False


def filter_black_ring_colors(colors, yolo_objs):
    if not colors or not yolo_objs:
        return colors

    digit_objs = [o for o in yolo_objs if o.get("label") in VALID_LABELS]
    if not digit_objs:
        return colors

    filtered = []
    for color in colors:
        if color.get("color_id") != 5:
            filtered.append(color)
            continue

        ring_like = False
        for obj in digit_objs:
            digit_x = int(obj["x"] + obj["w"] / 2)
            digit_y = int(obj["y"] + obj["h"] / 2)
            dx = color["cx"] - digit_x
            dy = color["cy"] - digit_y
            limit = max(16, int(color["r"] * 0.80))
            if dx * dx + dy * dy <= limit * limit:
                ring_like = True
                break

        if not ring_like:
            filtered.append(color)

    return filtered


def pick_ring_target(holes, yolo_objs=None, color_targets=None):
    digit_anchor = None

    if yolo_objs:
        valid_objs = [o for o in yolo_objs if o.get("label") in VALID_LABELS]
        if valid_objs:
            obj = max(valid_objs, key=lambda o: o.get("score", 0))
            digit_anchor = {
                "cx": int(obj["x"] + obj["w"] / 2),
                "cy": int(obj["y"] + obj["h"] / 2),
                "r": max(8, min(115, int(max(obj["w"], obj["h"]) * 0.63))),
            }

    if not digit_anchor:
        return None

    # 颜色物块上的偶发数字误检不应触发黄圈。
    if digit_over_color(yolo_objs, color_targets):
        return None

    return digit_anchor


def stabilize_ring_target(prev, curr):
    if prev is None or curr is None:
        return curr

    return {
        "cx": curr["cx"],
        "cy": curr["cy"],
        "r": int(RING_R_ALPHA * curr["r"] + (1 - RING_R_ALPHA) * prev["r"]),
    }


# ========= 颜色显示 =========
COLOR_DISP = {
    1: image.Color.from_rgb(220, 30, 30),
    2: image.Color.from_rgb(255, 215, 0),
    3: image.Color.from_rgb(15, 110, 200),
    4: image.Color.from_rgb(35, 180, 70),
    5: image.COLOR_WHITE,
    6: image.Color.from_rgb(80, 190, 230),
}


def run():
    G = image.COLOR_GREEN
    R = image.COLOR_RED
    Y = image.COLOR_YELLOW
    W = image.COLOR_WHITE

    # ---- 加载 YOLO ----
    detector = None
    yolo_w, yolo_h = 224, 224

    try:
        detector = nn.YOLOv5(model=MODEL_PATH, dual_buff=True)
        yolo_w = detector.input_width()
        yolo_h = detector.input_height()
        print("[model] YOLOv5 loaded: %s (%dx%d)" % (MODEL_PATH, yolo_w, yolo_h))
        # 打印模型所有标签，方便核对
        if detector.labels:
            print("[model] labels: %s" % list(detector.labels))
        else:
            print("[model] no labels in model, will use class_id directly")
    except Exception as e:
        print("[model] YOLOv5 load failed: %s" % e)

    # ---- 摄像头 ----
    cam = camera.Camera(CAM_W, CAM_H)
    disp = display.Display()
    serial_link = create_serial_link(SERIAL)

    frame_cnt = 0
    mode_id = 0
    last_digits = []
    last_yolo_objs = []  # 缓存上一次 YOLO 检测框
    last_holes = []      # 缓存上一次圆检测结果
    last_ring_target = None
    ring_confirmed = False
    ring_vx = 0
    ring_vy = 0
    ring_miss_frames = 0
    try:
        while not app.need_exit():
            img = cam.read()
            frame_cnt += 1

            command_mode = serial_link.poll_mode()
            if command_mode is not None and command_mode != mode_id:
                mode_id = command_mode
                reset_cache()
                reset_ring_state()
                serial_link.reset_frame_counter()
                last_digits = []
                last_yolo_objs = []
                last_holes = []
                last_ring_target = None
                ring_confirmed = False
                ring_vx = ring_vy = 0
                ring_miss_frames = 0

            colors = []
            digits = []
            yolo_objs = []
            holes = []
            ring_target = None
            run_color = False
            run_yolo = False
            run_hole = False

            if 1 <= mode_id <= 6:
                # Modes 1..6 run only the selected color detector.
                run_color = True
                colors = detect_colors(img, mode_id)
                target = max(colors, key=lambda c: c.get("area", 0)) if colors else None
                serial_link.send_result(target, CAM_W, CAM_H)

                for color in colors:
                    col = COLOR_DISP.get(color["color_id"], G)
                    img.draw_circle(color["cx"], color["cy"], color["r"], col, thickness=3)
                    img.draw_string(color["cx"], color["cy"], color["name"], col)

            elif mode_id == 7:
                # Mode 7 runs only the existing YOLO + circle pipeline.
                digits = last_digits
                yolo_objs = last_yolo_objs
                run_yolo = (detector is not None) and (frame_cnt % YOLO_SKIP == 0)

                if run_yolo:
                    try:
                        yolo_img = img.resize(yolo_w, yolo_h)
                        objs = detector.detect(yolo_img, conf_th=DIGIT_CONF_TH, iou_th=0.45)
                        digits = []
                        yolo_objs = []

                        for obj in objs:
                            raw_label = detector.labels[obj.class_id] if detector.labels else str(obj.class_id)
                            score = obj.score if hasattr(obj, "score") else 0
                            bx = int(obj.x * CAM_W / yolo_w)
                            by = int(obj.y * CAM_H / yolo_h)
                            bw = int(obj.w * CAM_W / yolo_w)
                            bh = int(obj.h * CAM_H / yolo_h)
                            corrected_label, corrected_score = correct_digit_by_shape(
                                raw_label, bw, bh, score
                            )
                            if corrected_label in VALID_LABELS:
                                digits.append(corrected_label)
                            yolo_objs.append({
                                "x": bx, "y": by, "w": bw, "h": bh,
                                "label": corrected_label, "raw_label": raw_label,
                                "class_id": obj.class_id, "score": corrected_score,
                                "raw_score": score,
                            })

                        last_digits = digits
                        last_yolo_objs = yolo_objs
                    except Exception:
                        digits = last_digits
                        yolo_objs = last_yolo_objs

                has_digit_anchor = any(o.get("label") in VALID_LABELS for o in yolo_objs)
                run_hole = has_digit_anchor and (frame_cnt % HOLE_SKIP == HOLE_SKIP - 1)

                if run_hole:
                    img_cv = image.image2cv(img, False, False)
                    holes = detect_all(img_cv)
                    last_holes = holes
                    if any(h.get("inner") for h in holes):
                        ring_confirmed = True
                elif not has_digit_anchor:
                    holes = []
                    last_holes = []
                    ring_confirmed = False
                    if last_ring_target and ring_miss_frames < RING_MISS_KEEP_FRAMES:
                        ring_miss_frames += 1
                    else:
                        last_ring_target = None
                else:
                    holes = last_holes

                ring_target = (
                    None
                    if not ring_confirmed
                    else pick_ring_target(holes, yolo_objs, [])
                )
                if ring_target:
                    if last_ring_target:
                        dx = max(-12, min(12, ring_target["cx"] - last_ring_target["cx"]))
                        dy = max(-12, min(12, ring_target["cy"] - last_ring_target["cy"]))
                        ring_vx = int(0.4 * dx + 0.6 * ring_vx)
                        ring_vy = int(0.4 * dy + 0.6 * ring_vy)
                    ring_target = stabilize_ring_target(last_ring_target, ring_target)
                    last_ring_target = ring_target
                    ring_miss_frames = 0
                elif not run_hole and last_ring_target:
                    ring_target = {
                        "cx": max(0, min(CAM_W - 1, last_ring_target["cx"] + ring_vx)),
                        "cy": max(0, min(CAM_H - 1, last_ring_target["cy"] + ring_vy)),
                        "r": last_ring_target["r"],
                    }
                    ring_vx = int(ring_vx * 0.65)
                    ring_vy = int(ring_vy * 0.65)
                    last_ring_target = ring_target
                elif run_hole or not ring_confirmed:
                    last_ring_target = None

                serial_link.send_result(ring_target, CAM_W, CAM_H)
                if ring_target:
                    img.draw_circle(ring_target["cx"], ring_target["cy"], ring_target["r"], Y, thickness=2)
                    img.draw_cross(ring_target["cx"], ring_target["cy"], R, size=6, thickness=2)
                if digits:
                    img.draw_string(4, 220, "NUM: %s" % ",".join(digits), W, scale=1.5)
                    dbg = yolo_objs[0]
                    img.draw_string(4, 202, "ID:%d %.2f %s" % (
                        dbg["class_id"], dbg["raw_score"], dbg["raw_label"]), W, scale=1)

            else:
                # Mode 0 is idle and reports found=0.
                serial_link.send_result(None, CAM_W, CAM_H)

            img.draw_string(
                5, 5, "MODE:%d FPS:%d C:%d D:%d R:%d TX:%d" % (
                    mode_id, int(time.fps()), len(colors), len(digits),
                    len(holes), serial_link.tx_count,
                ), G,
            )
            disp.show(img)
    finally:
        close = getattr(cam, "close", None)
        if callable(close):
            close()
        close = getattr(disp, "close", None)
        if callable(close):
            close()
        serial_link.close()


if __name__ == "__main__":
    run()
