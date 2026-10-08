#ifndef __CAR_H
#define __CAR_H

#include "stm32f10x.h"

extern volatile float anglea;

/* dir=0：speed 为目标朝向角；其他方向的 location 为命令脉冲数。 */
void car(int dir, int speed, int location);

#endif
