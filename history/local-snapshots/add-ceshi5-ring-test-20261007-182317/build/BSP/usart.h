#ifndef __USART_H
#define __USART_H

#include "board.h"
#include "fifo.h"

/**********************************************************
***	Emm_V5.0步进闭环控制例程
***	编写作者：ZHANGDATOU
***	技术支持：张大头闭环伺服
***	淘宝店铺：https://zhangdatou.taobao.com
***	CSDN博客：http s://blog.csdn.net/zhangdatou666
***	qq交流群：262438510
**********************************************************/

extern __IO bool rxFrameFlag;
extern __IO uint8_t rxCmd[FIFO_SIZE];
extern __IO uint8_t rxCount;
void usart_SendCmd(__IO uint8_t *cmd, uint8_t len);
void usart_SendByte(uint16_t data);

/* Position feedback for the four wheels; bit 0..3 means address 1..4. */
extern volatile uint8_t wheel_reply_done_mask;
extern volatile uint8_t wheel_reply_ack_mask;
extern volatile uint8_t wheel_reply_error_addr;
extern volatile uint8_t wheel_reply_error_code;
void WheelReply_Begin(void);
void WheelReply_Arm(uint8_t addr);
void WheelReply_End(void);

#endif
