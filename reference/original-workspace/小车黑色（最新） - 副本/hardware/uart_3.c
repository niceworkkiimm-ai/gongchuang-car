#include "uart_3.h"
#include "delay.h"

volatile float global_angle = 0.0f;

static const uint8_t unlock_register[] = {0xFF, 0xAA, 0x69, 0x88, 0xB5};
static const uint8_t reset_z_axis[] = {0xFF, 0xAA, 0x76, 0x00, 0x00};
static const uint8_t set_output_200Hz[] = {0xFF, 0xAA, 0x03, 0x0B, 0x00};
static const uint8_t set_baudrate_115200[] = {0xFF, 0xAA, 0x04, 0x06, 0x00};
static const uint8_t save_settings[] = {0xFF, 0xAA, 0x00, 0x00, 0x00};
static const uint8_t restart_device[] = {0xFF, 0xAA, 0x00, 0xFF, 0x00};

static void Usart3_SendByte(uint8_t Byte) {
    USART_SendData(USART3, Byte); //将字节数据写入数据寄存器，写入后USART自动生成时序波形
    while (USART_GetFlagStatus(USART3, USART_FLAG_TXE) == RESET); //等待发送完成
}

static void Usart3_SendArray(const uint8_t *array, uint16_t length) 
	{
    for (uint16_t i = 0; i < length; i++) {
        Usart3_SendByte(array[i]);
    }
    Usart3_SendByte(0x0D); // 发送回车符
    Usart3_SendByte(0x0A); // 发送换行符
		delay_ms(200);
	
}

void Usart3_Init(void) 
{
    /*开启时钟*/
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE); //开启USART3的时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);  //开启GPIOB的时钟
 
    /*GPIO初始化*/
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure); //将P引脚初始化为复用推挽输出
 
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure); //将P引脚初始化为上拉输入
 
    /*USART初始化*/
    USART_InitTypeDef USART_InitStructure; //定义结构体变量
    USART_InitStructure.USART_BaudRate = 115200; //波特率
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None; //硬件流控制，不需要
    USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx; //模式，发送模式和接收模式均选择
    USART_InitStructure.USART_Parity = USART_Parity_No; //奇偶校验，不需要
    USART_InitStructure.USART_StopBits = USART_StopBits_1; //停止位，选择1位
    USART_InitStructure.USART_WordLength = USART_WordLength_8b; //字长，选择8位
    USART_Init(USART3, &USART_InitStructure); //将结构体变量交给USART_Init，配置USART3
 
    /*中断输出配置*/
    USART_ITConfig(USART3, USART_IT_RXNE, ENABLE); //开启串口接收数据的中断
 
    /*NVIC中断分组*/
 
    /*NVIC配置*/
    NVIC_InitTypeDef NVIC_InitStructure; //定义结构体变量
    NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn; //选择配置NVIC的USART3线
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE; //指定NVIC线路使能
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1; //指定NVIC线路的抢占优先级为1
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1; //指定NVIC线路的响应优先级为1
    NVIC_Init(&NVIC_InitStructure); //将结构体变量交给NVIC_Init，配置NVIC外设
 
    /*USART使能*/
    USART_Cmd(USART3, ENABLE); //使能USART3，串口开始运行
	
	
	    // 发送配置命令
		
    Usart3_SendArray(unlock_register, sizeof(unlock_register));
		delay_ms(400);
    Usart3_SendArray(reset_z_axis, sizeof(reset_z_axis));
    Usart3_SendArray(set_output_200Hz, sizeof(set_output_200Hz));
    Usart3_SendArray(set_baudrate_115200, sizeof(set_baudrate_115200));
    Usart3_SendArray(save_settings, sizeof(save_settings));
    Usart3_SendArray(restart_device, sizeof(restart_device));
	
}

/* 保留原 11 字节姿态帧边界，仅使用 0x55 0x53 航向包。 */
void USART3_IRQHandler(void)
{
    static uint8_t packet[11];
    static uint8_t count = 0;
    uint8_t data;
    uint8_t sum;
    uint8_t i;
    int16_t yaw;

    if (USART_GetITStatus(USART3, USART_IT_RXNE) == RESET)
        return;

    data = (uint8_t)USART_ReceiveData(USART3);
    if (count == 0)
    {
        if (data == 0x55)
            packet[count++] = data;
    }
    else if (count == 1 && data != 0x53 && data != 0x52)
    {
        count = (data == 0x55) ? 1 : 0;
    }
    else
    {
        packet[count++] = data;
        if (count == sizeof(packet))
        {
            sum = 0;
            for (i = 0; i < sizeof(packet) - 1; ++i)
                sum = (uint8_t)(sum + packet[i]);
            if (sum == packet[10] && packet[1] == 0x53)
            {
                yaw = (int16_t)(((uint16_t)packet[7] << 8) | packet[6]);
                global_angle = ((float)yaw / 32768.0f) * 180.0f;
            }
            count = 0;
        }
    }
    USART_ClearITPendingBit(USART3, USART_IT_RXNE);
}
