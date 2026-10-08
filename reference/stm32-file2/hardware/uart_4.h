#ifndef __UART_4_H
#define __UART_4_H
#include "stm32f10x.h"
#include "maixcam_protocol.h"

#define MAIXCAM_LINK_TIMEOUT_MS 1000U
#define MAIXCAM_ALIGN_TIMEOUT_MS 15000U
#define MAIXCAM_ALIGN_CONFIRM_FRAMES 2U

extern volatile uint8_t maixcam_found;
extern volatile int16_t maixcam_dx, maixcam_dy;
/* 兼容调试观察：x=160+dx，y=120+dy；是否有效必须看 found。 */
extern volatile int32_t x, y;
extern volatile int32_t error_x, error_y;
extern volatile uint8_t maixcam_mode;
extern volatile uint32_t maixcam_uart_errors;

void uart_init4(void);
void Usart_SendByte4(USART_TypeDef *port, uint8_t byte);
void MaixCAM_Tick20ms(void);
uint32_t MaixCAM_NowMs(void);
void MaixCAM_ResetTarget(void);
uint8_t MaixCAM_SetMode(uint8_t mode);
uint8_t MaixCAM_ColorFromAscii(uint8_t character);
uint8_t MaixCAM_GetTarget(MaixCAM_Target *target);
uint8_t MaixCAM_TargetFound(void);
#endif
