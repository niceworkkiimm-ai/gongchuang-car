# color_detector.py
# Color detection with low-cost ROI tracking between full scans.
from color_config import *


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
        "cx": b.cx(),
        "cy": b.cy(),
        "r": int((b.w() + b.h()) / 4),
        "color_id": cid,
        "name": cfg["name"],
        "area": area,
    }


def _detect_one_color(img, cid, roi=None, min_area_scale=1.0):
    cfg = COLOR_LAB[cid]
    min_area = int(AREA_LIMIT.get(cid, 500) * min_area_scale)
    if min_area < 80:
        min_area = 80
    max_area = MAX_AREA.get(cid, 30000)

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
    """Clear tracked color targets when the selected mode changes."""
    global _color_cache, _frame_cnt
    _color_cache = []
    _frame_cnt = max(0, NO_CACHE_FULL_SCAN_EVERY - 1)


def detect_colors(img, color_id=None):
    global _color_cache, _frame_cnt
    _frame_cnt += 1

    if not _color_cache and (_frame_cnt % NO_CACHE_FULL_SCAN_EVERY != 0):
        return []

    if _color_cache and (_frame_cnt % FULL_SCAN_EVERY != 0):
        tracked = []
        for c in _color_cache:
            roi = _make_roi(c)
            result = _detect_one_color(img, c["color_id"], roi, min_area_scale=0.35)
            if not result:
                roi = _make_roi(c, scale=2.4)
                result = _detect_one_color(img, c["color_id"], roi, min_area_scale=0.25)
            if result:
                tracked.append(result)

        if tracked:
            _color_cache = _dedup_results(tracked)
            return _color_cache

        return _color_cache

    results = []
    color_ids = [int(color_id)] if color_id is not None else COLOR_LAB
    for cid in color_ids:
        result = _detect_one_color(img, cid)
        if result:
            results.append(result)

    _color_cache = _dedup_results(results)
    return _color_cache
