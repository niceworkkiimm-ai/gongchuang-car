#ifndef __CAR_H
#define __CAR_H

#include "stm32f10x.h"


/* dir10: same turn direction as dir6, 90 degrees, fixed 1:5 wheel paths.
 * These constants also determine the straight-line deductions in task.c.
 * Preserve the calibrated 4450*4 spin equivalent when tuning route lengths.
 */
#define CAR_ARC_90_TURN_PULSES   (4450L * 4L)
#define CAR_ARC_90_INNER_PULSES  (CAR_ARC_90_TURN_PULSES / 2L)
#define CAR_ARC_90_OUTER_PULSES (5L * CAR_ARC_90_INNER_PULSES)
#define CAR_ARC_90_CENTER_PULSES (3L * CAR_ARC_90_INNER_PULSES)
/* 圆弧在转弯前后各占用16998脉冲；两弯共用直线需要扣两次。 */
#define CAR_ARC_90_TRIM_PULSES ((int)(CAR_ARC_90_CENTER_PULSES * 0.63661977236758134308 + 0.5))

/* Arc fault: 0=none, 1=no status reply, 2=disabled/stalled, 3=move timeout,
 * 4=invalid speed. A fault stops the wheels and holds the route until reset.
 */
extern volatile uint8_t car_arc_fault;
extern volatile uint8_t car_arc_fault_addr;
void car(int dir,int speed,int location);
void MoveForwardAndRotate(float location);
#endif
