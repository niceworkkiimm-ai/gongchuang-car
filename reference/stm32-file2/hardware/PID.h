#ifndef __PID_H
#define __PID_H

#include "stm32f10x.h"                  // Device header
float pid2(int16_t speed1,float tar1);

float pid3(int16_t speed1,float tar1);
int angle(float Angle,float Gyroy,float Mechanical_Angle);
#endif
