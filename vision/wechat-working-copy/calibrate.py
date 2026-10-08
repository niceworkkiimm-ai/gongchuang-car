# LAB 标定工具 —— 物料放画面中央，看串口 LAB 值
from maix import app, camera, display, image, uart, pinmap
import cv2
import numpy as np

pinmap.set_pin_function("A17", "UART0_RX")
pinmap.set_pin_function("A16", "UART0_TX")
ser = uart.UART("/dev/ttyS0", 115200)

cam = camera.Camera(320, 240)
disp = display.Display()

while not app.need_exit():
    img = cam.read()
    img_cv = image.image2cv(img, False, False)

    # 转 LAB（OpenCV LAB 的 AB 偏移 128）
    lab = cv2.cvtColor(img_cv, cv2.COLOR_BGR2LAB)
    h, w = lab.shape[:2]

    # 画面中央 30x30 区域
    cx, cy, s = w // 2, h // 2, 15
    roi = lab[cy - s:cy + s, cx - s:cx + s]
    L_raw, A_raw, B_raw = cv2.mean(roi)[:3]
    L = int(L_raw)
    A = int(A_raw) - 128
    B = int(B_raw) - 128

    # 画面
    img.draw_rect(cx - s, cy - s, s * 2, s * 2, image.COLOR_RED, thickness=2)
    img.draw_string(4, 4, f"L:{L} A:{A} B:{B}", image.COLOR_GREEN, scale=1.5)

    # 串口
    ser.write(f"L={L}, A={A}, B={B}\r\n".encode())

    disp.show(img)
