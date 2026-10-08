#ifndef __OPENMV_H
#define __OPENMV_H
#include "uart_4.h"

typedef enum {
    VISION_OK = 0,
    VISION_BAD_MODE,
    VISION_NO_TARGET,
    VISION_LINK_TIMEOUT,
    VISION_ALIGN_TIMEOUT
} VisionResult;

extern volatile VisionResult vision_last_result;
/* mode 使用数值 1..7；颜色为 1..6，圆环/数字为 7。失败后调用者不得继续抓放。 */
VisionResult Vision_Align(uint8_t mode, uint16_t tolerance_px);
VisionResult vision(uint8_t mode);
VisionResult WL_dingwei(uint8_t color_mode);
VisionResult WL_dingwei2(uint8_t color_mode);
#endif
