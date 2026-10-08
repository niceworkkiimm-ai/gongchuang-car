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
uint8_t x = 0;    
uint8_t y = 0;    
float x1 = 0;    
float y1 = 0;  
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
    /* 发送一个字节数据到USART */
    USART_SendData(pUSARTx, ch);

    /* 等待发送数据寄存器为空 */
    while (USART_GetFlagStatus(pUSARTx, USART_FLAG_TXE) == RESET);
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
{			static int n=0;
    if (USART_GetITStatus(UART4, USART_IT_RXNE)  != RESET)  // 修正了检查的 UART4
    {
		uart4_RxData = USART_ReceiveData(UART4);
		uart4_RxDataopenmv[n++] = uart4_RxData;
		
		if(uart4_RxDataopenmv[0]!=0xa3) n=0;             		//?D??μúò?????í·
	  if((n==2)&&(uart4_RxDataopenmv[1]!=0xb3)) n=0;    		//?D??μú?t????í·	

		if(n==4)                           			      //′ú±íò?×éêy?Y′?ê?íê±?
		{
			n = 0;			

				x = uart4_RxDataopenmv[2]; 
				y = uart4_RxDataopenmv[3]; 
				
				error_x=122-x;
				error_y=49-y;
			
//			Usart_SendByte4(UART4, x1);
//				Usart_SendByte4(UART4, y1);
			
		}
	
     USART_ClearITPendingBit(UART4, USART_IT_RXNE); // 正确清除 UART4 的中断标志  
    }
		
}



int data_test(int data[])
{
	if(data[0]!=0xa3) return 0;  //??í·
	if(data[1]!=0xb3) return 0;  //??í·
	
	return 1;
}

