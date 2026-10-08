k# task.py —— 前进 → 识别颜色 → 发编码 → 退回
# 不动你原来的任何代码

from maix import app, camera, display, image, time, uart, pinmap
from color_detector import detect_colors

CAM_W = 320
CAM_H = 240

pinmap.set_pin_function("A17", "UART0_RX")
pinmap.set_pin_function("A16", "UART0_TX")
ser = uart.UART("/dev/ttyS0", 115200)

COLOR_DISP = {
    1: image.Color.from_rgb(220, 30, 30),
    2: image.Color.from_rgb(255, 215, 0),
    3: image.Color.from_rgb(15, 110, 200),
    4: image.Color.from_rgb(35, 180, 70),
    5: image.COLOR_WHITE,
    6: image.Color.from_rgb(80, 190, 230),
}

HOLD = 10                 # 稳定帧数
BACK_DELAY_MS = 2000      # 发完编码后等多久发归位


def run():
    cam = camera.Camera(CAM_W, CAM_H)
    disp = display.Display()
    G = image.COLOR_GREEN

    state = "FORWARD"       # FORWARD → DETECT → DONE
    ser.write(b"FORWARD\r\n")

    last_code = "0"
    hold_cnt = 0
    done_time = 0

    while not app.need_exit():
        img = cam.read()
        results = detect_colors(img)

        # 当前编码
        if results:
            codes = [str(r["color_id"]) for r in results]
            code = ",".join(codes)
        else:
            code = "0"

        # 防抖
        if code == last_code:
            hold_cnt += 1
        else:
            hold_cnt = 1
            last_code = code

        # ---- 状态机 ----
        if state == "FORWARD":
            # 前进中，检测到颜色就锁定
            if hold_cnt >= HOLD and code != "0":
                ser.write((code + "\r\n").encode())
                state = "DONE"
                done_time = time.ticks_ms()

        elif state == "DONE":
            if time.ticks_ms() - done_time > BACK_DELAY_MS:
                ser.write(b"BACK\r\n")
                state = "IDLE"

        # ---- 画面 ----
        for r in results:
            c = COLOR_DISP.get(r["color_id"], G)
            img.draw_circle(r["cx"], r["cy"], r["r"], c, thickness=3)
            img.draw_string(r["cx"] + r["r"] + 4, r["cy"] - 8,
                            r["name"], c, scale=1)

        img.draw_string(4, 4, f"{state} code:{code} ({hold_cnt})", G, scale=1)
        disp.show(img)


if __name__ == "__main__":
    run()
