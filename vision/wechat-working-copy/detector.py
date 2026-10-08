# 圆形检测模块
import cv2
import numpy as np
from config import *

# 预计算射线方向
_RAY_DIRS = [(np.cos(a), np.sin(a)) for a in np.linspace(0, 2 * np.pi, RADIAL_RAYS, endpoint=False)]


def _find_circles_in_binary(binary, min_area, max_area, r_min, r_max, circ_min):
    contours, _ = cv2.findContours(binary, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
    circles = []
    for cnt in contours:
        area = cv2.contourArea(cnt)
        if area < min_area or area > max_area:
            continue
        (cx, cy), r = cv2.minEnclosingCircle(cnt)
        cx, cy, r = int(cx), int(cy), int(r)
        if r < r_min or r > r_max:
            continue
        circ = area / (np.pi * r * r + 1e-9)
        if circ < circ_min:
            continue
        if len(cnt) >= 5:
            try:
                ell = cv2.fitEllipse(cnt)
                (ecx, ecy), (ew, eh), _ = ell
                if min(ew, eh) / max(ew, eh) > 0.6:
                    cx, cy = int(ecx), int(ecy)
                    r = int((ew + eh) / 4.0)
            except:
                pass
        circles.append({"cx": cx, "cy": cy, "r": r, "area": area, "circ": circ})
    return circles


def _dedup(circles, dist_th=15):
    if len(circles) <= 1:
        return circles
    circles = sorted(circles, key=lambda c: c["circ"], reverse=True)
    keep, used = [], [False] * len(circles)
    for i, ci in enumerate(circles):
        if used[i]:
            continue
        keep.append(ci)
        for j in range(i + 1, len(circles)):
            if used[j]:
                continue
            cj = circles[j]
            d = np.sqrt((ci["cx"] - cj["cx"]) ** 2 + (ci["cy"] - cj["cy"]) ** 2)
            if d < dist_th:
                used[j] = True
    return keep


def detect_holes(img_cv):
    """阶段1：检测 3 个大圆孔"""
    gray = cv2.cvtColor(img_cv, cv2.COLOR_RGB2GRAY)
    blur = cv2.GaussianBlur(gray, (3, 3), 0)
    binary = cv2.adaptiveThreshold(blur, 255, cv2.ADAPTIVE_THRESH_MEAN_C,
                                    cv2.THRESH_BINARY_INV, ADAPTIVE_BLOCK_HOLE, ADAPTIVE_C_HOLE)
    raw = _find_circles_in_binary(binary, HOLE_MIN_AREA, CAM_W * CAM_H // 2,
                                  HOLE_RADIUS_MIN, HOLE_RADIUS_MAX, HOLE_CIRCULARITY)
    unique = _dedup(raw, dist_th=25)

    if len(unique) < 3:
        return unique

    big = sorted(unique, key=lambda c: c["area"], reverse=True)[:10]
    best, best_var = None, 9999
    for i in range(len(big) - 2):
        for j in range(i + 1, len(big) - 1):
            for k in range(j + 1, len(big)):
                rs = [big[i]["r"], big[j]["r"], big[k]["r"]]
                var = max(rs) - min(rs)
                if var < best_var:
                    best_var = var
                    best = [big[i], big[j], big[k]]

    if best is None:
        return unique

    best.sort(key=lambda c: c["cx"])
    ys = [c["cy"] for c in best]
    if max(ys) - min(ys) > 30:
        return unique

    labels = ["Hole-3", "Hole-2", "Hole-1"]
    return [{**c, "label": labels[idx]} for idx, c in enumerate(best)]


def detect_inner_rings(img_cv, hole):
    """阶段2：白圆质心定中心 + 径向最大梯度找边缘"""
    hx, hy, hr = hole["cx"], hole["cy"], hole["r"]
    crop_size = int(hr * CROP_PADDING * 2)
    if crop_size < 20:
        return None

    x1 = max(0, hx - crop_size // 2)
    y1 = max(0, hy - crop_size // 2)
    x2 = min(img_cv.shape[1], x1 + crop_size)
    y2 = min(img_cv.shape[0], y1 + crop_size)
    crop = img_cv[y1:y2, x1:x2]

    if crop.shape[0] < 10 or crop.shape[1] < 10:
        return None

    gray = cv2.cvtColor(crop, cv2.COLOR_RGB2GRAY)
    blur = cv2.GaussianBlur(gray, (3, 3), 0)
    h, w = blur.shape

    _, white = cv2.threshold(blur, 0, 255, cv2.THRESH_BINARY + cv2.THRESH_OTSU)
    mask = np.zeros_like(white)
    cv2.circle(mask, (w // 2, h // 2), int(min(w, h) * 0.4), 255, -1)
    white_center = cv2.bitwise_and(white, mask)
    M = cv2.moments(white_center)
    if M["m00"] > 10:
        cx = int(M["m10"] / M["m00"])
        cy = int(M["m01"] / M["m00"])
    else:
        cx, cy = w // 2, h // 2

    max_r = min(cx, cy, w - cx, h - cy) - 2
    if max_r < 3:
        return None

    radii = []
    for dx, dy in _RAY_DIRS:
        best_step, best_grad = 0, 0
        for step in range(2, max_r):
            px = int(cx + dx * step)
            py = int(cy + dy * step)
            px2 = int(cx + dx * (step - 1))
            py2 = int(cy + dy * (step - 1))
            grad = float(blur[py2, px2]) - float(blur[py, px])
            if grad > best_grad:
                best_grad = grad
                best_step = step
        if best_grad > 15:
            radii.append(best_step)

    if len(radii) < max(5, RADIAL_RAYS // 4):
        return None

    r = int(np.median(radii))
    if r < 2:
        return None

    cx += x1
    cy += y1
    return {"center": {"cx": cx, "cy": cy, "r": r}}


# 帧间缓存
_smooth_cache = {}
_holes_cache = []
_frame_cnt = 0


def reset_state():
    """Clear cached circle candidates when switching modes."""
    global _smooth_cache, _holes_cache, _frame_cnt
    _smooth_cache = {}
    _holes_cache = []
    _frame_cnt = 0


def estimate_inner_from_hole(img_cv, hole):
    ratio = globals().get("INNER_RADIUS_RATIO", 0.48)
    est_r = max(2, int(hole["r"] * ratio))
    return {
        "center": {
            "cx": hole["cx"],
            "cy": hole["cy"],
            "r": est_r,
        }
    }


def detect_all(img_cv):
    global _smooth_cache, _holes_cache, _frame_cnt
    _frame_cnt += 1

    if _frame_cnt % max(HOLE_REFRESH_EVERY, 1) == 0 or not _holes_cache:
        holes = detect_holes(img_cv)
        if holes:
            _holes_cache = holes

    results = []
    for hole in _holes_cache:
        label = hole.get("label", "?")
        inner = estimate_inner_from_hole(img_cv, hole)

        if inner is not None:
            if label in _smooth_cache:
                prev = _smooth_cache[label]
                max_jump = globals().get("INNER_MAX_JUMP", 45)
                dx = inner["center"]["cx"] - prev["cx"]
                dy = inner["center"]["cy"] - prev["cy"]
                dist = np.sqrt(dx * dx + dy * dy)
                if dist > max_jump:
                    scale = max_jump / (dist + 1e-9)
                    inner["center"]["cx"] = int(prev["cx"] + dx * scale)
                    inner["center"]["cy"] = int(prev["cy"] + dy * scale)
                inner["center"]["cx"] = int(SMOOTH_ALPHA_POS * inner["center"]["cx"] + (1 - SMOOTH_ALPHA_POS) * prev["cx"])
                inner["center"]["cy"] = int(SMOOTH_ALPHA_POS * inner["center"]["cy"] + (1 - SMOOTH_ALPHA_POS) * prev["cy"])
                inner["center"]["r"]  = int(SMOOTH_ALPHA_R   * inner["center"]["r"]  + (1 - SMOOTH_ALPHA_R)   * prev["r"])
            _smooth_cache[label] = {
                "cx": inner["center"]["cx"],
                "cy": inner["center"]["cy"],
                "r":  inner["center"]["r"],
            }
        elif label in _smooth_cache:
            prev = _smooth_cache[label]
            inner = {"center": {"cx": prev["cx"], "cy": prev["cy"], "r": prev["r"]}}

        results.append({"hole": hole, "inner": inner})
    return results
