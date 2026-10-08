"""通过 MaixCAM Pro 的 B3 引脚控制补光灯。"""


class FillLight:
    def __init__(self):
        from maix import err, gpio, pinmap

        pin_name = "B3"
        err.check_raise(
            pinmap.set_pin_function(pin_name, "GPIOB3"),
            "补光灯引脚映射失败",
        )
        self._led = gpio.GPIO(pin_name, gpio.Mode.OUT)
        self._led.value(0)
        self._level = 0

    def set_mode(self, mode_id):
        """模式 1～7 开灯，模式 0 关灯；电平不变时不重复写入。"""
        level = int(1 <= mode_id <= 7)
        if level != self._level:
            self._led.value(level)
            self._level = level

    def close(self):
        """程序退出时主动关灯，不依赖对象回收时的引脚状态。"""
        self._led.value(0)
        self._level = 0
