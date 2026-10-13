#ifndef __CAR_H
#define __CAR_H

#include "stm32f10x.h"


/* dir10 pulse targets: dir6-compatible direction, 1:5 wheel paths.
 * These constants also determine the straight-line deductions in task.c.
 * Preserve the calibrated 4450*4 spin equivalent when tuning route lengths.
 */
#define CAR_ARC_90_TURN_PULSES   (4450L * 4L)
#define CAR_ARC_90_INNER_PULSES  (CAR_ARC_90_TURN_PULSES / 2L)
#define CAR_ARC_90_OUTER_PULSES (5L * CAR_ARC_90_INNER_PULSES)
#define CAR_ARC_90_CENTER_PULSES (3L * CAR_ARC_90_INNER_PULSES)
/* 既有直线补偿基准16998脉冲；两弯共用直线扣两次，实际轨迹需试车调距。 */
#define CAR_ARC_90_TRIM_PULSES ((int)(CAR_ARC_90_CENTER_PULSES * 0.63661977236758134308 + 0.5))

void car(int dir,int speed,int location);
void MoveForwardAndRotate(float location);
#endif
