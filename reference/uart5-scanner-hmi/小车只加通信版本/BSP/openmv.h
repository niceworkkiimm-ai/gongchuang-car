#ifndef __OPENMV_H
#define __OPENMV_H

#include "stm32f10x.h"

extern int error_x;
extern int error_y;
void  vision(char task);
void WL_dingwei2(char WL);
void WL_dingwei(char WL);
float calc_speed(float error, float Kp, float max_speed);

#endif
