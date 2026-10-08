# 圆形检测 —— 所有可调参数

# 摄像头
CAM_W = 320
CAM_H = 240

# 模式 1～6：连续真实检测帧的中心稳定后，才允许输出有效位置。
# 3 像素是试用容差，须在实物停稳和转入时上板验证。
COLOR_STABLE_FRAMES = 5
COLOR_STABLE_TOLERANCE_PX = 3

# UART0: Maix A16 TX / A17 RX
SERIAL = {
    "device": "/dev/ttyS0",
    "baudrate": 115200,
    "frame_interval": 10,
    "accept_ascii": True,
}

# ---- 阶段1：大圆孔检测 ----
HOLE_MIN_AREA = 80
HOLE_CIRCULARITY = 0.60
HOLE_RADIUS_MIN = 8
HOLE_RADIUS_MAX = 170
ADAPTIVE_BLOCK_HOLE = 31
ADAPTIVE_C_HOLE = 5

# ---- 阶段2：孔内精检 ----
CROP_PADDING = 0.80
INNER_RADIUS_RATIO = 0.56
INNER_MAX_JUMP = 90
SMOOTH_ALPHA_POS = 0.95
SMOOTH_ALPHA_R   = 0.45
HOLE_REFRESH_EVERY = 1
RADIAL_RAYS = 18
