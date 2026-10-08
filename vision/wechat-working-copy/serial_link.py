"""UART mode commands and fixed-size vision result frames."""


class SerialLink:
    FRAME_HEADER = 0xAA
    FRAME_TAIL = 0x55

    def __init__(self, config=None):
        from maix import pinmap, uart

        self.config = dict(config or {})
        pinmap.set_pin_function("A16", "UART0_TX")
        pinmap.set_pin_function("A17", "UART0_RX")
        self._serial = uart.UART(
            self.config.get("device", "/dev/ttyS0"),
            int(self.config.get("baudrate", 115200)),
        )
        self._frame_interval = max(1, int(self.config.get("frame_interval", 10)))
        self._frame_count = 0
        self.tx_count = 0
        self.last_command = None

    @staticmethod
    def _as_bytes(data):
        if data is None:
            return b""
        if isinstance(data, int):
            return bytes((data & 0xFF,))
        if isinstance(data, str):
            return data.encode("ascii", "ignore")
        try:
            return bytes(data)
        except (TypeError, ValueError):
            return b""

    def poll_mode(self):
        """Read available bytes and return the newest mode command 0..7."""
        try:
            data = self._as_bytes(self._serial.read())
        except Exception:
            return None

        mode = None
        accept_ascii = bool(self.config.get("accept_ascii", True))
        for value in data:
            if 0 <= value <= 7:
                mode = value
            elif accept_ascii and ord("0") <= value <= ord("7"):
                mode = value - ord("0")
        if mode is not None:
            self.last_command = mode
        return mode

    @staticmethod
    def _signed_u16(value):
        value = max(-32768, min(32767, int(value)))
        return value & 0xFFFF

    @classmethod
    def encode_result(cls, found, dx=0, dy=0):
        """Build AA found dxH dxL dyH dyL xor 55."""
        found_byte = 1 if found else 0
        dx_value = cls._signed_u16(dx)
        dy_value = cls._signed_u16(dy)
        payload = [
            found_byte,
            (dx_value >> 8) & 0xFF,
            dx_value & 0xFF,
            (dy_value >> 8) & 0xFF,
            dy_value & 0xFF,
        ]
        checksum = 0
        for value in payload:
            checksum ^= value
        return bytes([cls.FRAME_HEADER] + payload + [checksum, cls.FRAME_TAIL])

    @staticmethod
    def _result_state(result):
        if result is None:
            return False, 0, 0
        if isinstance(result, dict):
            return bool(result.get("valid", True)), int(result.get("cx", 0)), int(result.get("cy", 0))
        return (
            bool(getattr(result, "valid", False)),
            int(getattr(result, "cx", 0)),
            int(getattr(result, "cy", 0)),
        )

    def send_result(self, result, frame_width, frame_height):
        """Transmit one result frame every configured number of vision frames."""
        self._frame_count += 1
        if self._frame_count < self._frame_interval:
            return False
        self._frame_count = 0

        found, center_x, center_y = self._result_state(result)
        if found:
            dx = center_x - int(frame_width) // 2
            dy = center_y - int(frame_height) // 2
        else:
            dx, dy = 0, 0
        self._serial.write(self.encode_result(found, dx, dy))
        self.tx_count += 1
        return True

    def reset_frame_counter(self):
        self._frame_count = 0

    def close(self):
        close = getattr(self._serial, "close", None)
        if callable(close):
            close()


def create_serial_link(config=None):
    return SerialLink(config)
