#ifndef GYRO_DEBUG_H
#define GYRO_DEBUG_H

#include "stm32f10x.h"

/* 1: gyro diagnostic page; 0: previous scanner/camera OLED pages. */
#define GYRO_OLED_DEBUG_ENABLE 1

extern volatile uint32_t gyro_yaw_frame_count;
extern volatile int16_t car_debug_mode;
extern volatile int16_t car_debug_target;
extern volatile uint8_t car_debug_active;
void Gyro_DisplayPoll(void);

#endif
