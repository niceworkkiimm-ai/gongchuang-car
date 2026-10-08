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
         "lab":[0,80,40,80,10,80]
     },

    2:{
         "name":"Yellow",
         "lab":[60,71,-11,-4,35,120]
     },
    3:{
         "name":"Blue",
         "lab":[4,25,4,24,-42,-23]
     },
    4:{
         "name":"Green",
         "lab":[19,39,-30,-11,-13,33]
     },
    5:{
         "name":"Black",
         "lab":[0,14,-4,6,-11,7]
     },
    6:{
         "name":"Cyan",
         "lab":[21,38,-13,4,-51,-18]
     }

}
