#ifndef __UART_4_H
#define __UART_4_H
#include "stdio.h"	
#include "sys.h" 

#include "stm32f10x.h"

//如果想串口中断接收，请不要注释以下宏定义


void uart_init4();
extern  uint8_t uart4_RxData;              // UART5 接收到的数据
extern  uint8_t uart4_RxDataopenmv[10];              // UART5 接收到的数据
extern uint8_t WL_flag;  
extern int32_t x;    
extern int32_t y;    
extern float x1;    
extern float y1;  
void Usart_SendByte4( USART_TypeDef * pUSARTx, uint8_t ch);
void Usart_SendArray( USART_TypeDef * pUSARTx, uint8_t *array, uint16_t num);
void Usart_SendString4( USART_TypeDef * pUSARTx, char *str);
void Usart_SendHalfWord( USART_TypeDef * pUSARTx, uint16_t ch);
int data_test(int data[]);
void UsartPrintf(USART_TypeDef *USARTx, char *fmt,...);


extern volatile uint8_t maixcam_found, maixcam_mode;
extern volatile int16_t maixcam_dx, maixcam_dy;

#endif

