#include "usart.h"

/**********************************************************
***	Emm_V5.0步进闭环控制例程
***	编写作者：ZHANGDATOU
***	技术支持：张大头闭环伺服
***	淘宝店铺：https://zhangdatou.taobao.com
***	CSDN博客：http s://blog.csdn.net/zhangdatou666
***	qq交流群：262438510
**********************************************************/

__IO bool rxFrameFlag = false;
__IO uint8_t rxCmd[FIFO_SIZE] = {0};
__IO uint8_t rxCount = 0;
/**
	* @brief   USART1中断函数
	* @param   无
	* @retval  无
	*/

/* Parsed at byte reception so adjacent motor replies cannot overwrite each other.
 * Active only during WL_dingwei wheel positioning. The legacy FIFO stays intact.
 * FD replies carry no transaction number: never overlap two tracked moves.
 */
volatile uint8_t wheel_reply_done_mask = 0;
volatile uint8_t wheel_reply_ack_mask = 0;
volatile uint8_t wheel_reply_error_addr = 0;
volatile uint8_t wheel_reply_error_code = 0;
static volatile uint8_t wheel_reply_active = 0;
static volatile uint8_t wheel_reply_armed_mask = 0;
static uint8_t wheel_reply_bytes[4];
static uint8_t wheel_reply_count = 0;

void WheelReply_Begin(void)
{
    uint32_t mask = __get_PRIMASK();
    __disable_irq();
    wheel_reply_done_mask = 0;
    wheel_reply_ack_mask = 0;
    wheel_reply_error_addr = 0;
    wheel_reply_error_code = 0;
    wheel_reply_armed_mask = 0;
    wheel_reply_count = 0;
    wheel_reply_active = 1;
    __set_PRIMASK(mask);
}

void WheelReply_Arm(uint8_t addr)
{
    uint32_t mask;
    if (addr < 1 || addr > 4) return;
    mask = __get_PRIMASK();
    __disable_irq();
    wheel_reply_armed_mask |= (uint8_t)(1U << (addr - 1));
    __set_PRIMASK(mask);
}

void WheelReply_End(void)
{
    uint32_t mask = __get_PRIMASK();
    __disable_irq();
    wheel_reply_active = 0;
    wheel_reply_armed_mask = 0;
    wheel_reply_count = 0;
    __set_PRIMASK(mask);
}

static void WheelReply_Feed(uint8_t data)
{
    uint8_t addr, status, bit;
    if (!wheel_reply_active) return;
    wheel_reply_bytes[wheel_reply_count++] = data;
    if (wheel_reply_count < 4) return;
    addr = wheel_reply_bytes[0];
    status = wheel_reply_bytes[2];
    if (addr >= 1 && addr <= 4 && wheel_reply_bytes[1] == 0xFD &&
        wheel_reply_bytes[3] == 0x6B &&
        (status == 0x02 || status == 0x9F || status == 0xE2 ||
         status == 0xEE || status == 0x12 || status == 0x22))
    {
        bit = (uint8_t)(1U << (addr - 1));
        if (wheel_reply_armed_mask & bit)
        {
            if (status == 0x9F) wheel_reply_done_mask |= bit;
            else if (status == 0x02) wheel_reply_ack_mask |= bit;
            else if (!wheel_reply_error_code)
            {
                wheel_reply_error_addr = addr;
                wheel_reply_error_code = status;
            }
        }
        wheel_reply_count = 0;
    }
    else
    {
        /* Slide one byte after noise, malformed tails, or another function. */
        wheel_reply_bytes[0] = wheel_reply_bytes[1];
        wheel_reply_bytes[1] = wheel_reply_bytes[2];
        wheel_reply_bytes[2] = wheel_reply_bytes[3];
        wheel_reply_count = 3;
    }
}

static void WheelReply_RxError(void)
{
    wheel_reply_count = 0;
    if (wheel_reply_active && !wheel_reply_error_code)
    {
        wheel_reply_error_addr = 0; /* UART error, no reliable motor address. */
        wheel_reply_error_code = 0xFF;
    }
}

void USART1_IRQHandler(void)
{
	__IO uint16_t i = 0;
    uint16_t sr;
    uint8_t data;

/**********************************************************
***	串口接收中断
**********************************************************/
	if(USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)
	{
		// 未完成一帧数据接收，数据进入缓冲队列
		sr = USART1->SR;
        data = (uint8_t)USART1->DR;
        fifo_enQueue(data);
        if (sr & (USART_FLAG_ORE | USART_FLAG_NE | USART_FLAG_FE | USART_FLAG_PE))
            WheelReply_RxError();
        else
            WheelReply_Feed(data);

		// 清除串口接收中断
		USART_ClearITPendingBit(USART1, USART_IT_RXNE);
	}

/**********************************************************
***	串口空闲中断
**********************************************************/
	else if(USART_GetITStatus(USART1, USART_IT_IDLE) != RESET)
	{
		// 先读SR再读DR，清除IDLE中断
		USART1->SR; USART1->DR;

		// 提取一帧数据命令
		rxCount = fifo_queueLength(); for(i=0; i < rxCount; i++) { rxCmd[i] = fifo_deQueue(); }
		// 一帧数据接收完成，置位帧标志位
		rxFrameFlag = true;
	}
}

/**
	* @brief   USART发送多个字节
	* @param   无
	* @retval  无
	*/
void usart_SendCmd(__IO uint8_t *cmd, uint8_t len)
{
	__IO uint8_t i = 0;
	
	for(i=0; i < len; i++) { usart_SendByte(cmd[i]); }
}

/**
	* @brief   USART发送一个字节
	* @param   无
	* @retval  无
	*/
void usart_SendByte(uint16_t data)
{
	__IO uint16_t t0 = 0;
	
	USART1->DR = (data & (uint16_t)0x01FF);

	while(!(USART1->SR & USART_FLAG_TXE))
	{
		++t0; if(t0 > 8000)	{	return; }
	}
}


