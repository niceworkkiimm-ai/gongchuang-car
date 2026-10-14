#ifndef __UART_3_H
#define __UART_3_H

#include <stdio.h>
#include <stdint.h>
/* 若实测新模块的角度正负方向与原车相反，改为 -1.0f。 */
#define GYRO_YAW_DIRECTION 1.0f

void Usart3_Init(void);
void Usart3_SendByte(uint8_t Byte);
extern volatile float anglea;
void Usart3_SendString(char *String);
void Usart3_SendArray(uint8_t *array, uint16_t length);

void Usart3_Printf(char *format, ...);
void ParseAndPrintData(uint8_t *data, uint16_t length);
uint8_t CalculateChecksum(uint8_t *data, uint16_t length, uint8_t type);
#endif
