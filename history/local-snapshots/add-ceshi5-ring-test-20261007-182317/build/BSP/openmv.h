#ifndef __OPENMV_H
#define __OPENMV_H

#include "stm32f10x.h"

extern volatile int error_x;
extern volatile int error_y;
void  vision(char task);
void vision_at(char task, int center_x, int center_y);
void WL_dingwei2(char WL);
void WL_dingwei(char WL);
float calc_speed(float error, float Kp, float max_speed);

/* Diagnostic state for the locator; DONE means the fixed delay elapsed.
 * Motor arrival and camera alignment are not confirmed by this state. */
#define WL_WHEEL_IDLE 0U
#define WL_WHEEL_WAIT 1U
#define WL_WHEEL_DONE 2U
/* Total positioning window after the first recognized target, not per axis. */
#define WL_ALIGN_DELAY_MS 2000U
extern volatile uint8_t wl_wheel_wait_state;
extern volatile char wl_wheel_wait_axis;

#endif
