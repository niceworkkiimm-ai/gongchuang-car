#include "sys.h"
#include "uart_5.h"
#include "stdio.h"
#include "string.h"
#include "stdarg.h"
#include "stm32f10x_tim.h"
#include "stm32f10x.h"                  // Device header
#include "uart_4.h"
///////////////////////////////////////////////////////
/*
引脚占用：
UART5_TX        PC12                                                                                                                                                          
UART5_RX        PD2
用于OpenMv_1串口通信
*/
///////////////////////////////////////////////////////
int cnt = 0;
int shunxuma = 0;
uint8_t saoma_data[1000];


volatile uint8_t uart5_RxData;              // UART5 接收到的数据
void uart_init5()
{

    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    /* 开启UART5，GPIOC和GPIOD的时钟 */
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_UART5,ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOD,ENABLE);
    
    /* 复位串口5 */
    USART_DeInit(UART5);

    /*--------------------------GPIO端配置-----------------------------*/
    //UART5_TX   GPIOC.12   复用推排输出
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOC, &GPIO_InitStructure);

    //UART5_RX   GPIOD.2  浮空输入
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOD, &GPIO_InitStructure);

    /*--------------------------串口中断优先级配置-----------------------------*/
    //UART5 NVIC 配置
    NVIC_InitStructure.NVIC_IRQChannel = UART5_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3 ; //抢占优先级3
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;      //子优先级3
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;         //IRQ通道使能
    NVIC_Init(&NVIC_InitStructure); //根据指定的参数初始化VIC存在器

    /*--------------------------串口的工作参数配置-----------------------------*/
    //UART 初始化设置
    USART_InitStructure.USART_BaudRate = 9600;//串口波特率
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;//字长为8位数据格式
    USART_InitStructure.USART_StopBits = USART_StopBits_1;//一个停止位
    USART_InitStructure.USART_Parity = USART_Parity_No;//无奇偶校验位
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//无硬件数据流控制
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx; //收发模式

    USART_Init(UART5, &USART_InitStructure); //初始化串口5
    USART_ITConfig(UART5, USART_IT_RXNE, ENABLE);//开启串口5接收中断
    USART_Cmd(UART5, ENABLE);                    //使能串口5


}

/*****************  发送一个字符 **********************/
void Usart_SendByte5(USART_TypeDef *pUSARTx, uint8_t ch)
{
    /* 发送一个字节数据到USART */
    USART_SendData(pUSARTx, ch);

    /* 等待发送数据存在器为空 */
    while (USART_GetFlagStatus(pUSARTx, USART_FLAG_TXE) == RESET);
}

/****************** 发送8位的数组 ************************/
void Usart_SendArray5(USART_TypeDef *pUSARTx, uint8_t *array, uint16_t num)
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
void Usart_SendString5(USART_TypeDef *pUSARTx, char *str)
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
void Usart_SendHalfWord5(USART_TypeDef *pUSARTx, uint16_t ch)
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
void UsartPrintf5(USART_TypeDef *USARTx, char *fmt, ...)
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


void UART5_IRQHandler(void)    //扫码数组
{
    if (USART_GetITStatus(UART5, USART_IT_RXNE) == SET)  // 修正了检查的 UART5
    {
        uart5_RxData = USART_ReceiveData(UART5);
     saoma_data[cnt++] = uart5_RxData;  
 		//GM65模块发完一组数据后会自动发送一个回车符，所以通过检测是否接受到回车来判断数据是否接收完成
 		if(uart5_RxData == 0x0D) 
 		{
			shunxuma = 1;
 		}
		
		
		
		
		
		
	if(shunxuma == 1)
	{
//		Usart_SendByte4(UART4,saoma_data[0]);
//		Usart_SendByte4(UART4,saoma_data[1]);
//		Usart_SendByte4(UART4,saoma_data[2]);
//		Usart_SendByte4(UART4,saoma_data[4]);
//		Usart_SendByte4(UART4,saoma_data[5]);
//		Usart_SendByte4(UART4,saoma_data[6]);
//			
		
		Usart_SendString5(UART5,"t0.txt=");	
		Usart_SendString5(UART5,"\"");
		
        Usart_SendByte5(UART5,saoma_data[0]);
		Usart_SendByte5(UART5,saoma_data[1]);
		Usart_SendByte5(UART5,saoma_data[2]);
		Usart_SendByte5(UART5,saoma_data[3]);
		Usart_SendByte5(UART5,saoma_data[4]);
		Usart_SendByte5(UART5,saoma_data[5]);
		Usart_SendByte5(UART5,saoma_data[6]);
		Usart_SendString5(UART5,"\"");
		
		Usart_SendByte5(UART5,0xFF);
        Usart_SendByte5(UART5,0xFF);
		Usart_SendByte5(UART5,0xFF);
			
		
			
		}
		shunxuma =10;
	

        USART_ClearITPendingBit(UART5, USART_IT_RXNE);  // 正确清除 UART5 的中断标志
    }
}
