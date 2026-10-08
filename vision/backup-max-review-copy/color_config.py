# 摄像头分辨率
CAM_W = 320
CAM_H = 240


# 最小识别面积
MIN_AREA = 200


# LAB阈值
# 格式：
# [L_min,L_max,A_min,A_max,B_min,B_max]


COLOR_LAB ={

    1:{
         "name":"Red",
         "lab":[0,50,40,80,10,40]
     },

    2:{
         "name":"Yellow",
         "lab":[37,70,-30,19,48,80]
     },
    3:{
         "name":"Blue",
         "lab":[10,30,5,25,-47,-32]
     },
    4:{
         "name":"Green",
         "lab":[20,65,-33,-10,-10,30]
     },
    5:{
         "name":"Black",
         "lab":[0,30,-10,5,-6,7]
     },
    6:{
         "name":"Cyan",
         "lab":[25,45,-10,9,-34,-10]
     }

}

# 仅 Blue/Cyan 使用 HSV；原 COLOR_LAB 全部保留，其他颜色继续原生 LAB 检测。
# OpenCV 范围：H=0~179，S/V=0~255。
# 格式：[H_min, H_max, S_min, S_max, V_min, V_max]。
# 这是初始阈值，须用 calibrate_hsv.py 测量真实物料后调整。
HSV_COLOR_IDS = (3, 6)
COLOR_HSV = {
    3: {
        "name": "Blue",
        "hsv": [112, 134, 128, 255, 91, 255],
    },
    6: {
        "name": "Cyan",
        "hsv": [97, 110, 136, 255, 110, 255],
    },
}
