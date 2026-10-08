#include "uart_5.h"
#include "sys.h"
#include "uart_4.h"
#include "stdio.h"
#include "string.h"
#include "stdarg.h"
#include "stm32f10x_tim.h"
#include "stm32f10x.h"
#include "usart.h"
#include "PID.h" 
#include "openmv.h"
#include "camera_debug.h"



//#include "fifo.h"
///////////////////////////////////////////////////////
/*
引脚占用：
UART4_TX        PC10                                                                                                                                                          
UART4_RX	    PC11
用于OpenMv_1串口通信
*/
///////////////////////////////////////////////////////
//extern __IO uint8_t rxCmd[FIFO_SIZE];
 uint8_t uart4_RxDataopenmv[10];
 uint8_t uart4_RxData = 0;              // UART4 接收到的数据
 uint8_t WL_flag = 0;  
int32_t x = 0;    
int32_t y = 0;    
float x1 = 0;    
float y1 = 0;  
/* Camera protocol state only; no motor commands or timing dependencies. */
volatile uint8_t maixcam_found = 0;
volatile int16_t maixcam_dx = 0, maixcam_dy = 0;
volatile uint8_t maixcam_mode = 0;
static uint8_t camera_frame[8];
static uint8_t camera_count = 0;
static volatile uint8_t legacy_material_status = 0;
/* Receive telemetry only; it does not change positioning or protocol. */
volatile uint32_t camera_debug_rx_bytes = 0;
volatile uint32_t camera_debug_frames = 0;
volatile uint32_t camera_debug_bad_frames = 0;
volatile uint32_t camera_debug_uart_errors = 0;
volatile uint32_t camera_debug_last_frame_ms = 0;
#define CAMERA_DEBUG_DWT_CTRL   (*(volatile uint32_t *)0xE0001000UL)
#define CAMERA_DEBUG_DWT_CYCCNT (*(volatile uint32_t *)0xE0001004UL)
static uint8_t camera_debug_clock_started;
static uint32_t camera_debug_clock_cycles;
static uint32_t camera_debug_clock_ms;
static uint32_t camera_debug_clock_remainder;

uint32_t CameraDebug_NowMs(void)
{
    uint32_t mask, now, delta, per_ms, milliseconds;
    mask = __get_PRIMASK();
    __disable_irq();
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    CAMERA_DEBUG_DWT_CTRL |= 1UL; /* Share DWT without resetting its counter. */
    now = CAMERA_DEBUG_DWT_CYCCNT;
    per_ms = SystemCoreClock / 1000UL;
    if (!camera_debug_clock_started)
    {
        camera_debug_clock_started = 1;
        camera_debug_clock_cycles = now;
    }
    delta = (uint32_t)(now - camera_debug_clock_cycles);
    camera_debug_clock_cycles = now;
    if (per_ms)
    {
        camera_debug_clock_ms += delta / per_ms;
        camera_debug_clock_remainder += delta % per_ms;
        camera_debug_clock_ms += camera_debug_clock_remainder / per_ms;
        camera_debug_clock_remainder %= per_ms;
    }
    milliseconds = camera_debug_clock_ms;
    __set_PRIMASK(mask);
    return milliseconds;
}


static int16_t Camera_Signed16(uint8_t high, uint8_t low)
{
    int32_t value = ((uint32_t)high << 8) | low;
    if (value >= 32768L) value -= 65536L;
    return (int16_t)value;
}

static void Camera_ReceiveByte(uint8_t byte)
{
    uint8_t i, check;
    if (camera_count == 0 && byte != 0xAA) return;
    camera_frame[camera_count++] = byte;
    if (camera_count < 8) return;
    check = camera_frame[1] ^ camera_frame[2] ^ camera_frame[3] ^
            camera_frame[4] ^ camera_frame[5];
    if (camera_frame[0] != 0xAA || camera_frame[1] > 1 ||
        camera_frame[6] != check || camera_frame[7] != 0x55)
    {
        ++camera_debug_bad_frames;
        for (i = 1; i < 8 && camera_frame[i] != 0xAA; ++i) {}
        camera_count = (uint8_t)(8 - i);
        if (camera_count) memmove(camera_frame, camera_frame + i, camera_count);
        return;
    }
    camera_count = 0;
    maixcam_found = camera_frame[1];
    maixcam_dx = Camera_Signed16(camera_frame[2], camera_frame[3]);
    maixcam_dy = Camera_Signed16(camera_frame[4], camera_frame[5]);
    x = maixcam_found ? 160L + maixcam_dx : 0;
    y = maixcam_found ? 120L + maixcam_dy : 0;
    /* Preserve the ORIGINAL alignment reference, including no-target values. */
    error_x = 155 - x;
    error_y = 125 - y;
    /* Compatibility data for unchanged task.c; found is NOT a stopped flag. */
    uart4_RxDataopenmv[0] = maixcam_found ?
        (legacy_material_status ? legacy_material_status : 0xA3) : 0;
    uart4_RxDataopenmv[1] = 0xB3;
    camera_debug_last_frame_ms = CameraDebug_NowMs();
    ++camera_debug_frames;
}

void uart_init4()
{
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    /* 开启UART4，GPIOC的时钟 */
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_UART4,ENABLE);
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC,ENABLE);
	
    /* 复位串口4 */
    USART_DeInit(UART4);

    /*--------------------------GPIO端配置-----------------------------*/
    //UART4_TX   GPIOC.10   复用推挽输出
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOC, &GPIO_InitStructure);

    //UART4_RX   GPIOC.11  浮空输入
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOC, &GPIO_InitStructure);

    /*--------------------------串口中断优先级配置-----------------------------*/
    //UART4 NVIC 配置
    NVIC_InitStructure.NVIC_IRQChannel = UART4_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3 ; //抢占优先级3
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;      //子优先级3
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;         //IRQ通道使能
    NVIC_Init(&NVIC_InitStructure); //根据指定的参数初始化VIC寄存器

    /*--------------------------串口的工作参数配置-----------------------------*/
    //UART 初始化设置
    USART_InitStructure.USART_BaudRate = 115200;//串口波特率
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;//字长为8位数据格式
    USART_InitStructure.USART_StopBits = USART_StopBits_1;//一个停止位
    USART_InitStructure.USART_Parity = USART_Parity_No;//无奇偶校验位
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//无硬件数据流控制
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx; //收发模式

    USART_Init(UART4, &USART_InitStructure); //初始化串口4
    USART_ITConfig(UART4, USART_IT_RXNE, ENABLE);//开启串口4接收中断
    USART_Cmd(UART4, ENABLE);                    //使能串口4
//		Usart_SendString4(UART4, "task");


}

/*****************  发送一个字符 **********************/
void Usart_SendByte4(USART_TypeDef *pUSARTx, uint8_t ch)
{
    if (pUSARTx == UART4)
    {
        uint8_t mode = ch;
        uint8_t legacy = 0;
        uint32_t mask;
        if (ch >= '0' && ch <= '3') mode = (uint8_t)(ch - '0');
        else if (ch >= '4' && ch <= '6') mode = 7; /* legacy ring commands */
        else if (ch >= '7' && ch <= '9')
        {
            mode = (uint8_t)(ch - '7' + 1);
            legacy = (uint8_t)(0xA4 + ch - '7');
        }
        if (mode > 7) return;
        mask = __get_PRIMASK();
        __disable_irq();
        legacy_material_status = legacy;
        uart4_RxDataopenmv[0] = 0;
        camera_count = 0;
        maixcam_mode = mode;
        __set_PRIMASK(mask);
        ch = mode; /* raw numeric byte 00..07, not ASCII */
    }

    /* 发送一个字节数据到USART */
    USART_SendData(pUSARTx, ch);

    /* 等待发送数据寄存器为空 */
    while (USART_GetFlagStatus(pUSARTx, USART_FLAG_TXE) == RESET);
    if (pUSARTx == UART4)
    {
        scanner_camera_tx_mode = ch;
        scanner_camera_tx_valid = 1;
    }
}

/****************** 发送8位的数组 ************************/
void Usart_SendArray(USART_TypeDef *pUSARTx, uint8_t *array, uint16_t num)
{
    uint8_t i;

    for (i = 0; i < num; i++)
    {
        /* 发送一个字节数据到USART */
        Usart_SendByte(pUSARTx, array[i]);

    }
    /* 等待发送完成 */
    while (USART_GetFlagStatus(pUSARTx, USART_FLAG_TC) == RESET);
}

/*****************  发送字符串 **********************/
void Usart_SendString4(USART_TypeDef *pUSARTx, char *str)
{
    unsigned int k = 0;
    do
    {
        Usart_SendByte(pUSARTx, *(str + k));
        k++;
    }
    while (*(str + k) != '\0');

    /* 等待发送完成 */
    while (USART_GetFlagStatus(pUSARTx, USART_FLAG_TC) == RESET)
    {}
}

/*****************  发送一个16位数 **********************/
void Usart_SendHalfWord(USART_TypeDef *pUSARTx, uint16_t ch)
{
    uint8_t temp_h, temp_l;

    /* 取出高八位 */
    temp_h = (ch & 0XFF00) >> 8;
    /* 取出低八位 */
    temp_l = ch & 0XFF;

    /* 发送高八位 */
    USART_SendData(pUSARTx, temp_h);
    while (USART_GetFlagStatus(pUSARTx, USART_FLAG_TXE) == RESET);

    /* 发送低八位 */
    USART_SendData(pUSARTx, temp_l);
    while (USART_GetFlagStatus(pUSARTx, USART_FLAG_TXE) == RESET);
}

/*
************************************************************
*	函数名称：	UsartPrintf
*
*	函数功能：	格式化打印
*
*	入口参数：	USARTx：串口组
*				fmt：不定长参
*
*	返回参数：	无
*
*	说明：		
************************************************************
*/
// 移植该函数的时候一定要加上头文件：#include "stdarg.h"，va函数在头文件stdarg.h中，要使用其中的参数，必须包含此头文件#include "stdarg.h"
void UsartPrintf(USART_TypeDef *USARTx, char *fmt, ...)
{

    unsigned char UsartPrintfBuf[296];
    va_list ap;
    unsigned char *pStr = UsartPrintfBuf;

    va_start(ap, fmt);
    vsnprintf((char *)UsartPrintfBuf, sizeof(UsartPrintfBuf), fmt, ap);                         //格式化
    va_end(ap);

    while (*pStr != 0)
    {
        USART_SendData(USARTx, *pStr++);
        while (USART_GetFlagStatus(USARTx, USART_FLAG_TC) == RESET);
    }
}


void UART4_IRQHandler(void)
{
    uint32_t status = UART4->SR;
    if (status & (USART_FLAG_PE | USART_FLAG_FE | USART_FLAG_NE | USART_FLAG_ORE))
    {
        (void)UART4->DR;
        if (status & USART_FLAG_RXNE) ++camera_debug_rx_bytes;
        ++camera_debug_uart_errors;
        camera_count = 0;
        return;
    }
    if (status & USART_FLAG_RXNE)
    {
        uart4_RxData = (uint8_t)UART4->DR;
        ++camera_debug_rx_bytes;
        Camera_ReceiveByte(uart4_RxData);
    }
}

int data_test(int data[])
{
	if(data[0]!=0xa3) return 0;  //??í·
	if(data[1]!=0xb3) return 0;  //??í·
	
	return 1;
}

