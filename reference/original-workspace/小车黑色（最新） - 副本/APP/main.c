#include "board.h"
#include "delay.h"
#include "uart_3.h"
#include "heading.h"
#include "task.h"

int main(void)
{
    board_init();       /* USART1：四个轮电机。 */
    Usart3_Init();       /* USART3：行走朝向反馈。 */
    Heading_Init();     /* TIM2：每 20 ms 更新朝向，不输出舵机 PWM。 */
    delay_ms(1000);

    walk_route();       /* 只执行一次；路线和距离在 BSP/task.c 修改。 */

    while (1)
    {
    }
}
