#ifndef __UART_5_H
#define __UART_5_H
#include "stdio.h"	
#include "sys.h" 



//如果想串口中断接收，请不要注释以下宏定义
extern uint8_t saoma_data[1000];
void uart_init5();
extern volatile uint8_t uart5_RxData;              // UART5 接收到的数据
void Usart_SendByte5( USART_TypeDef * pUSARTx, uint8_t ch);
void Usart_SendArray5( USART_TypeDef * pUSARTx, uint8_t *array, uint16_t num);
void Usart_SendString5( USART_TypeDef * pUSARTx, char *str);
void Usart_SendHalfWord5( USART_TypeDef * pUSARTx, uint16_t ch);

void UsartPrintf5(USART_TypeDef *USARTx, char *fmt,...);


#endif
