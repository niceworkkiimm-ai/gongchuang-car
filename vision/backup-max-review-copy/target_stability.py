"""物料中心稳定门控：只接受连续帧的真实检测结果，不预测或复用旧目标。"""


class StableTargetLock:
    def __init__(self, frames=5, tolerance_px=3):
        if type(frames) is not int or frames < 1:
            raise ValueError("稳定帧数必须是大于 0 的整数")
        if type(tolerance_px) is not int or tolerance_px < 0:
            raise ValueError("坐标容差必须是非负整数像素")
        self.frames = frames
        self.tolerance_px = tolerance_px
        self.reset()

    def reset(self):
        """换模式、丢失或中断连续检测时清空锁定。"""
        self._points = []
        self._last_frame = None
        self._color_id = None

    def update(self, target, frame_id):
        """每幅相机图像调用一次；未稳定返回 None，稳定返回当前帧目标。"""
        if target is None or not target.get("valid", True):
            self.reset()
            return None

        color_id = target["color_id"]
        if (self._last_frame is not None and frame_id != self._last_frame + 1) or (
            self._color_id is not None and color_id != self._color_id
        ):
            self.reset()

        self._last_frame = frame_id
        self._color_id = color_id
        point = (int(target["cx"]), int(target["cy"]))
        self._points.append(point)
        if len(self._points) > self.frames:
            self._points.pop(0)

        xs = [p[0] for p in self._points]
        ys = [p[1] for p in self._points]
        if (max(xs) - min(xs) > self.tolerance_px or
                max(ys) - min(ys) > self.tolerance_px):
            # 当前帧作为新一组的第 1 帧，旧位置不能参与下次锁定。
            self._points = [point]
            return None

        if len(self._points) < self.frames:
            return None
        # 不冻结、不平均坐标；显示与串口均使用本帧的物料几何中心。
        return dict(target)
