#ifndef CAMERA_DEBUG_H
#define CAMERA_DEBUG_H
#include "stm32f10x.h"
/* 1: camera receive diagnostics; 0: restore the previous selected OLED page. */
#define CAMERA_OLED_DEBUG_ENABLE 1
extern volatile uint32_t camera_debug_rx_bytes;
extern volatile uint32_t camera_debug_frames;
extern volatile uint32_t camera_debug_bad_frames;
extern volatile uint32_t camera_debug_uart_errors;
extern volatile uint32_t camera_debug_last_frame_ms;
uint32_t CameraDebug_NowMs(void);
#endif
