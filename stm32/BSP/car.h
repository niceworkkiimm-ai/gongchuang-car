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
 * 前段/转弯四轮统一速度模式；前段用电机位置反馈测距，转弯用陀螺仪。
 */
#define CAR_DIR13_INNER_PULSES       (2225L * 4L)
/* 2225:9000只决定转弯速度比例，不再给单侧轮子下位置终点。 */
#define CAR_DIR13_OUTER_PULSES       (9000L * 4L)
/* Emm默认1.8度、16细分的串口脉冲单位：一圈3200脉冲。 */
#define CAR_DIR13_PULSES_PER_REV     3200UL
#define CAR_DIR13_TURN_ACC           235
#define CAR_DIR13_EXIT_SETTLE_MS     100
#define CAR_DIR13_AFTER_PULSES       (11750L * 4L - 2L * CAR_ARC_90_TRIM_PULSES)
#define CAR_DIR13_TURN_DEG           90.0f
#define CAR_DIR13_GYRO_TIMEOUT_MS    500UL
#define CAR_DIR13_MOVE_TIMEOUT_MS    12000UL
/* phase: 1=前直行，2=转弯，3=后直行，4=完成，255=停止；error见car.c。 */
extern volatile uint8_t car13_phase;
extern volatile uint8_t car13_error;

/* dir12: location=直行脉冲，内部再加2225*4/9000*4的转弯脉冲。 */
void car(int dir,int speed,int location);
void MoveForwardAndRotate(float location);
#endif
