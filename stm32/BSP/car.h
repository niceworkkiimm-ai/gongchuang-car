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

/* dir13：前直行距离由car第三参数传入；后直行和转向在这里调。
 * 2/4外侧轮在转弯期间保持速度运动；用现有陀螺仪判断转向量。
 */
#define CAR_DIR13_INNER_PULSES       (2225L * 4L)
#define CAR_DIR13_AFTER_PULSES       (11750L * 4L - 2L * CAR_ARC_90_TRIM_PULSES)
#define CAR_DIR13_TURN_DEG           90.0f
#define CAR_DIR13_GYRO_TIMEOUT_MS    500UL
#define CAR_DIR13_MOVE_TIMEOUT_MS    12000UL
/* phase: 1=直行转弯，2=后直行，3=结束，255=停止；error见car.c。 */
extern volatile uint8_t car13_phase;
extern volatile uint8_t car13_error;

/* dir12: location=直行脉冲，内部再加2225*4/9000*4的转弯脉冲。 */
void car(int dir,int speed,int location);
void MoveForwardAndRotate(float location);
#endif
