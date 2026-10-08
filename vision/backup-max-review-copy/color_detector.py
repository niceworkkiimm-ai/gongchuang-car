# color_detector.py
# LAB/HSV 混合检测，保留全图扫描之间的低成本 ROI 跟踪。
import cv2
import numpy as np
from maix import image

from color_config import CAM_W, CAM_H, COLOR_LAB, COLOR_HSV, HSV_COLOR_IDS


AREA_LIMIT = {
    1: 1200,  # Red
    2: 600,  # Yellow
    3: 1000,  # Blue
    4: 1000,  # Green
    5: 900,  # Black
    6: 500,  # Cyan
}

MAX_AREA = {
    1: 30000,
    2: 30000,
    3: 30000,
    4: 30000,
    5: 15000,
    6: 30000,
}

MIN_SIDE = {
    1: 16,
    2: 16,
    3: 14,
    4: 14,
    5: 10,
    6: 10,
}

COLOR_RATIO_MIN = 0.35
COLOR_RATIO_MAX = 2.80
COLOR_FILL_MIN = 0.25

BLUE_ID = 3
BLUE_RATIO_MIN = 0.50
BLUE_RATIO_MAX = 2.00
BLUE_FILL_MIN = 0.35

CYAN_ID = 6
CYAN_RATIO_MIN = 0.45
CYAN_RATIO_MAX = 2.20
CYAN_FILL_MIN = 0.28
CYAN_MAX_AREA = 18000

BLACK_ID = 5
BLACK_MAX_SIDE = 85
BLACK_RATIO_MIN = 0.40
BLACK_RATIO_MAX = 2.50
BLACK_FILL_MIN = 0.52
BLACK_FILL_MAX = 1.00

FULL_SCAN_EVERY = 12
NO_CACHE_FULL_SCAN_EVERY = 6
ROI_PADDING = 34

_color_cache = []
_frame_cnt = 0

_HSV_KERNEL = np.ones((3, 3), dtype=np.uint8)


def _build_hsv_bounds():
    """启动时检查范围，预建 inRange 边界；调参后重启程序。"""
    bounds = {}
    for cid in HSV_COLOR_IDS:
        values = COLOR_HSV[cid]["hsv"]
        if len(values) != 6 or any(type(v) is not int for v in values):
            raise ValueError("HSV 阈值必须是 6 个整数，颜色 ID=%s" % cid)
        h0, h1, s0, s1, v0, v1 = values
        if not (0 <= h0 <= h1 <= 179 and 0 <= s0 <= s1 <= 255 and 0 <= v0 <= v1 <= 255):
            raise ValueError("HSV 阈值越界或上下限颠倒，颜色 ID=%s" % cid)
        bounds[cid] = (np.array([h0, s0, v0], dtype=np.uint8),
                       np.array([h1, s1, v1], dtype=np.uint8))
    blue_h = COLOR_HSV[BLUE_ID]["hsv"][:2]
    cyan_h = COLOR_HSV[CYAN_ID]["hsv"][:2]
    if max(blue_h[0], cyan_h[0]) <= min(blue_h[1], cyan_h[1]):
        raise ValueError("Blue 与 Cyan 的 Hue 范围不能重叠（上下限均包含）")
    return bounds


_HSV_BOUNDS = _build_hsv_bounds()


def _to_hsv(img):
    # ensure_bgr=True 且 copy=True 才保证 RGB888 相机帧转换为真正 BGR。
    # copy=False 会禁用 ensure_bgr；不能照搬旧标定工具的 False, False。
    bgr = image.image2cv(img, True, True)
    if bgr.dtype != np.uint8 or bgr.ndim != 3 or bgr.shape[2] != 3:
        raise ValueError("HSV 检测要求三通道 uint8 BGR 图像")
    return cv2.cvtColor(bgr, cv2.COLOR_BGR2HSV)


def _detect_hsv_color(hsv, cid, min_area, max_area, roi=None):
    """在共享 HSV 图像中检测；ROI 局部中心加偏移后返回全图坐标。"""
    height, width = hsv.shape[:2]
    x0, y0, x1, y1 = 0, 0, width, height
    if roi is not None:
        x, y, w, h = map(int, roi)
        if w <= 0 or h <= 0:
            return None
        x0, y0 = max(0, x), max(0, y)
        x1, y1 = min(width, x + w), min(height, y + h)
        if x1 <= x0 or y1 <= y0:
            return None
    lower, upper = _HSV_BOUNDS[cid]
    mask = cv2.inRange(hsv[y0:y1, x0:x1], lower, upper)
    mask = cv2.morphologyEx(mask, cv2.MORPH_OPEN, _HSV_KERNEL)
    mask = cv2.morphologyEx(mask, cv2.MORPH_CLOSE, _HSV_KERNEL)
    # OpenCV 3 返回 image/contours/hierarchy，4+ 返回 contours/hierarchy。
    contours = cv2.findContours(mask, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)[-2]
    min_side = MIN_SIDE.get(cid, 14)
    if cid == BLUE_ID:
        ratio_min, ratio_max, fill_min = BLUE_RATIO_MIN, BLUE_RATIO_MAX, BLUE_FILL_MIN
    else:
        ratio_min, ratio_max, fill_min = CYAN_RATIO_MIN, CYAN_RATIO_MAX, CYAN_FILL_MIN
        max_area = min(max_area, CYAN_MAX_AREA)
    best = None
    best_area = -1.0
    for contour in contours:
        area = float(cv2.contourArea(contour))
        if area < min_area or area > max_area or area <= best_area:
            continue
        x, y, w, h = cv2.boundingRect(contour)
        if w < min_side or h < min_side:
            continue
        if not ratio_min <= w / h <= ratio_max or area / (w * h) < fill_min:
            continue
        # 使用外接矩形几何中心，并还原 ROI 到整幅图像的坐标。
        best = {
            "cx": int(x0 + x + w // 2),
            "cy": int(y0 + y + h // 2),
            "r": int((w + h) / 4),
            "color_id": int(cid),
            "name": COLOR_HSV[cid]["name"],
            "area": int(area),
        }
        best_area = area
    return best


def _find_blobs(img, cfg, min_area, roi=None):
    if roi:
        try:
            return img.find_blobs(
                [cfg["lab"]],
                pixels_threshold=min_area,
                area_threshold=min_area,
                merge=False,
                roi=roi,
            )
        except TypeError:
            return None

    return img.find_blobs(
        [cfg["lab"]],
        pixels_threshold=min_area,
        area_threshold=min_area,
        merge=False,
    )


def _blob_to_result(cid, cfg, b, min_area, max_area):
    area = b.area()
    w = b.w()
    h = b.h()

    if w <= 0 or h <= 0:
        return None
    if area < min_area or area > max_area:
        return None

    min_side = MIN_SIDE.get(cid, 14)
    if w < min_side or h < min_side:
        return None

    ratio = w / h
    fill = area / (w * h)

    if cid == BLUE_ID:
        if ratio < BLUE_RATIO_MIN or ratio > BLUE_RATIO_MAX:
            return None
        if fill < BLUE_FILL_MIN:
            return None
    elif cid == CYAN_ID:
        if area > CYAN_MAX_AREA:
            return None
        if ratio < CYAN_RATIO_MIN or ratio > CYAN_RATIO_MAX:
            return None
        if fill < CYAN_FILL_MIN:
            return None
    elif cid == BLACK_ID:
        if max(w, h) > BLACK_MAX_SIDE:
            return None
        if ratio < BLACK_RATIO_MIN or ratio > BLACK_RATIO_MAX:
            return None
        if fill < BLACK_FILL_MIN or fill > BLACK_FILL_MAX:
            return None
    else:
        if ratio < COLOR_RATIO_MIN or ratio > COLOR_RATIO_MAX:
            return None
        if fill < COLOR_FILL_MIN:
            return None

    return {
        # find_blobs 的外接矩形位置已经是整幅图像坐标。
        "cx": int(b.x() + w // 2),
        "cy": int(b.y() + h // 2),
        "r": int((b.w() + b.h()) / 4),
        "color_id": int(cid),
        "name": cfg["name"],
        "area": int(area),
    }


def _detect_one_color(img, cid, roi=None, min_area_scale=1.0, hsv=None):
    min_area = int(AREA_LIMIT.get(cid, 500) * min_area_scale)
    if min_area < 80:
        min_area = 80
    max_area = MAX_AREA.get(cid, 30000)

    if cid in HSV_COLOR_IDS:
        # detect_colors 始终传入本次调用共用的 HSV，不为两种颜色或重试重复转换。
        if hsv is None:
            hsv = _to_hsv(img)
        return _detect_hsv_color(hsv, cid, min_area, max_area, roi)

    cfg = COLOR_LAB[cid]
    blobs = _find_blobs(img, cfg, min_area, roi)
    if not blobs:
        return None

    blobs = sorted(blobs, key=lambda x: x.area(), reverse=True)
    for b in blobs:
        result = _blob_to_result(cid, cfg, b, min_area, max_area)
        if result:
            return result

    return None


def _make_roi(c, scale=1.6):
    pad = max(ROI_PADDING, int(c["r"] * scale))
    x = max(0, int(c["cx"] - pad))
    y = max(0, int(c["cy"] - pad))
    w = min(CAM_W - x, int(pad * 2))
    h = min(CAM_H - y, int(pad * 2))
    if w <= 5 or h <= 5:
        return None
    return (x, y, w, h)


def _dedup_results(results):
    if len(results) <= 1:
        return results

    keep = []
    for i, a in enumerate(results):
        same = False

        # 黑色阴影容易和相邻的蓝/浅蓝物块连成一个 blob，重叠时优先保留彩色物块。
        if a["color_id"] == BLACK_ID:
            for b in results:
                if b["color_id"] == BLACK_ID:
                    continue
                dis = ((a["cx"] - b["cx"]) ** 2 + (a["cy"] - b["cy"]) ** 2) ** 0.5
                if dis < max(20, int((a["r"] + b["r"]) * 0.85)):
                    same = True
                    break
            if same:
                continue

        for j, b in enumerate(results):
            if i == j:
                continue

            dis = ((a["cx"] - b["cx"]) ** 2 + (a["cy"] - b["cy"]) ** 2) ** 0.5
            if dis < 20 and a["area"] < b["area"]:
                same = True
                break

        if not same:
            keep.append(a)

    return keep


def reset_cache():
    """切换模式时清空目标；下一次调用立即进行全图扫描。"""
    global _color_cache, _frame_cnt
    _color_cache = []
    _frame_cnt = max(0, NO_CACHE_FULL_SCAN_EVERY - 1)


def detect_colors(img, color_id=None):
    """仅返回本帧真实检测结果；缓存只用于下一帧 ROI 搜索。"""
    global _color_cache, _frame_cnt
    color_ids = [int(color_id)] if color_id is not None else list(COLOR_LAB)
    if any(cid not in COLOR_LAB for cid in color_ids):
        raise ValueError("颜色 ID 必须为 1~6，模式 0/7 由 main.py 处理")
    # main.py 本来就会 reset_cache；也防止其他调用方切换 ID 后沿用旧颜色缓存。
    if color_id is not None and any(c["color_id"] not in color_ids for c in _color_cache):
        reset_cache()
    _frame_cnt += 1

    if not _color_cache and (_frame_cnt % NO_CACHE_FULL_SCAN_EVERY != 0):
        return []

    tracking = bool(_color_cache) and (_frame_cnt % FULL_SCAN_EVERY != 0)
    active_ids = [c["color_id"] for c in _color_cache] if tracking else color_ids
    # LAB 专用帧/空缓存跳帧完全不做 OpenCV 转换；其余每次调用最多转换一次。
    hsv = _to_hsv(img) if any(cid in HSV_COLOR_IDS for cid in active_ids) else None

    if tracking:
        tracked = []
        for c in _color_cache:
            roi = _make_roi(c)
            result = _detect_one_color(img, c["color_id"], roi, min_area_scale=0.35, hsv=hsv)
            if not result:
                roi = _make_roi(c, scale=2.4)
                result = _detect_one_color(img, c["color_id"], roi, min_area_scale=0.25, hsv=hsv)
            if result:
                tracked.append(result)

        if tracked:
            _color_cache = _dedup_results(tracked)
            return _color_cache

        # 两次 ROI 跟踪均失败：立即丢弃旧位置，禁止把缓存当成静止目标。
        reset_cache()
        return []

    results = []
    for cid in color_ids:
        result = _detect_one_color(img, cid, hsv=hsv)
        if result:
            results.append(result)

    _color_cache = _dedup_results(results)
    return _color_cache
