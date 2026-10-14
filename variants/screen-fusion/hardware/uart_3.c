#include "gyro_log.h"
#include "stm32f10x.h"                  // Device header
#include <stdio.h>
#include <stdarg.h>
#include "uart_3.h"
#include "gyro_debug.h"
#include "delay.h"
 
 
extern volatile float global_angle;
volatile uint32_t gyro_yaw_frame_count = 0;
extern volatile uint8_t new_data_received;
extern volatile float angular_velocity_y;
extern volatile float angular_velocity_z;
extern uint8_t received_data_packet[11];
extern volatile float anglea;

/* 单轴陀螺仪：115200/8N1，5A AA 角速度，5A BB 航向角。 */
static uint8_t unlock_register[] = {0x55, 0xAA, 0x13, 0x8E, 0x5F};
static uint8_t reset_z_axis[] = {0x55, 0xAA, 0x15, 0x00, 0x00};
static uint8_t set_output_200Hz[] = {0x55, 0xAA, 0x02, 0x0B, 0x00};
static uint8_t save_settings[] = {0x55, 0xAA, 0x00, 0x00, 0x00};
#define GYRO_FRAME_LENGTH 5U
static uint8_t gyro_rx_buffer[GYRO_FRAME_LENGTH];
static uint8_t gyro_rx_count;

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
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2); //配置NVIC为分组2
 
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
		
    /* 上电后保持车身静止；手册启动时间为500ms，预留600ms。 */
    delay_ms(600);
    Usart3_SendArray(unlock_register, sizeof(unlock_register));
    delay_ms(100);
    Usart3_SendArray(set_output_200Hz, sizeof(set_output_200Hz));
    delay_ms(100);
    Usart3_SendArray(reset_z_axis, sizeof(reset_z_axis));
    delay_ms(100);
    Usart3_SendArray(save_settings, sizeof(save_settings));
    delay_ms(100);
    /* 默认波特率已是115200；保存后直接接收，不额外重启模块。 */
}

void Usart3_SendByte(uint8_t Byte) {
    USART_SendData(USART3, Byte); //将字节数据写入数据寄存器，写入后USART自动生成时序波形
    while (USART_GetFlagStatus(USART3, USART_FLAG_TXE) == RESET); //等待发送完成
}
 
void Usart3_SendString(char *String) {
    uint8_t i = 0;
    while (String[i]) { // 遍历字符数组（字符串），遇到字符串结束标志位后停止
        Usart3_SendByte(String[i]); // 依次调用Usart3_SendByte发送每个字节数据
        i++;
    }
}
 
void Usart3_Printf(char *format, ...) {
    char String[100]; //定义字符数组
    va_list arg; //定义可变参数列表数据类型的变量arg
    va_start(arg, format); //从format开始，接收参数列表到arg变量
    vsprintf(String, format, arg); //使用vsprintf打印格式化字符串和参数列表到字符数组中
    va_end(arg); //结束变量arg
    Usart3_SendString(String); //串口发送字符数组（字符串）
}
 
void Usart3_SendArray(uint8_t *array, uint16_t length)
{
    uint16_t i;
    /* 二进制命令严格按长度发送，不追加回车、换行。 */
    for (i = 0; i < length; ++i)
        Usart3_SendByte(array[i]);
    while (USART_GetFlagStatus(USART3, USART_FLAG_TC) == RESET);
}

static uint8_t Gyro_FrameValid(uint8_t *data, uint16_t length)
{
    if (length != GYRO_FRAME_LENGTH || data[0] != 0x5A)
        return 0;
    if (data[1] != 0xAA && data[1] != 0xBB)
        return 0;
    return CalculateChecksum(data, GYRO_FRAME_LENGTH - 1U, data[1]) == data[4];
}

static void Gyro_ReceiveByte(uint8_t data)
{
    uint8_t i;
    if (gyro_rx_count == 0 && data != 0x5A)
        return;
    gyro_rx_buffer[gyro_rx_count++] = data;
    if (gyro_rx_count < GYRO_FRAME_LENGTH)
        return;

    if (Gyro_FrameValid(gyro_rx_buffer, GYRO_FRAME_LENGTH))
    {
        ParseAndPrintData(gyro_rx_buffer, GYRO_FRAME_LENGTH);
        gyro_rx_count = 0;
    }
    else
    {
        /* 错帧滑动一字节重新找头，保留可能已收到的下一帧开头。 */
        for (i = 1; i < GYRO_FRAME_LENGTH; ++i)
            gyro_rx_buffer[i - 1U] = gyro_rx_buffer[i];
        gyro_rx_count = GYRO_FRAME_LENGTH - 1U;
    }
}

void USART3_IRQHandler(void)
{
    uint16_t status = USART3->SR;
    uint8_t data;
    if (status & (USART_SR_RXNE | USART_SR_ORE | USART_SR_FE |
                  USART_SR_NE | USART_SR_PE))
    {
        /* 先读SR再读DR，清除接收及错误状态；不使用出错字节。 */
        data = (uint8_t)USART3->DR;
        if (status & (USART_SR_ORE | USART_SR_FE | USART_SR_NE | USART_SR_PE))
        {
            gyro_rx_count = 0;
            return;
        }
        Gyro_ReceiveByte(data);
    }
}

void ParseAndPrintData(uint8_t *data, uint16_t length)
{
    int16_t raw;
    float yaw;
    if (!Gyro_FrameValid(data, length))
        return;
    raw = (int16_t)(((uint16_t)data[3] << 8) | data[2]);

    if (data[1] == 0xBB)
    {
        yaw = GYRO_YAW_DIRECTION * (float)raw / 32768.0f * 180.0f;
        if (yaw >= 180.0f) yaw -= 360.0f;
        else if (yaw < -180.0f) yaw += 360.0f;
        global_angle = yaw;
        ++gyro_yaw_frame_count;
        GyroLog_OnYaw();
    }
    else
    {
        angular_velocity_z = GYRO_YAW_DIRECTION * (float)raw / 32768.0f * 2000.0f;
    }
    new_data_received = 1;
}

uint8_t CalculateChecksum(uint8_t *data, uint16_t length, uint8_t type)
{
    uint16_t i;
    uint8_t checksum = 0;
    (void)type;
    for (i = 0; i < length; ++i)
        checksum = (uint8_t)(checksum + data[i]);
    return checksum;
}
