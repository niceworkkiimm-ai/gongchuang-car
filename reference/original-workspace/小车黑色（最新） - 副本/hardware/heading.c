#include "heading.h"
#include "car.h"
#include "uart_3.h"

/* 保留原 TIM2 的 20 ms 更新周期，删除 PA0/PA1 舵机 PWM 输出。 */
void Heading_Init(void)
{
    TIM_TimeBaseInitTypeDef timer;
    NVIC_InitTypeDef irq;

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
    TIM_InternalClockConfig(TIM2);
    TIM_TimeBaseStructInit(&timer);
    timer.TIM_Prescaler = 72 - 1;
    timer.TIM_Period = 20000 - 1;
    TIM_TimeBaseInit(TIM2, &timer);
    TIM_ClearITPendingBit(TIM2, TIM_IT_Update);

    irq.NVIC_IRQChannel = TIM2_IRQn;
    irq.NVIC_IRQChannelPreemptionPriority = 1;
    irq.NVIC_IRQChannelSubPriority = 0;
    irq.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&irq);
    TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);
    TIM_Cmd(TIM2, ENABLE);
}

void TIM2_IRQHandler(void)
{
    if (TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET)
    {
        /* 等效于原 angle(global_angle, 0, 0)，保留整数化朝向。 */
        anglea = (int)global_angle;
        TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
    }
}
