#include "task.h"
#include "car.h"
#include "delay.h"

/* 保留原 yuanliao() 在首次视觉定位之前的两段行走。 */
void walk_route(void)
{
    const int speed = 500;  /* 电机转速，RPM。 */

    car(8, speed, 6000 * 4);   /* 第一段：24000 个命令脉冲。 */
    delay_ms(500);
    car(0, 0, 0);             /* 朝向修正到 0 度。 */
    delay_ms(50);

    car(1, speed, 16500 * 4);  /* 第二段：66000 个命令脉冲。 */
    car(0, 0, 0);             /* 朝向修正并停止四轮。 */
    delay_ms(50);
}
