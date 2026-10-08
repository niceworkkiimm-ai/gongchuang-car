#include "stm32f10x.h"                  // Device header
#include <stdio.h>
#include <stdarg.h>
#include "uart_3.h"
#include "delay.h"
 
 
extern volatile float global_angle;
extern volatile uint8_t new_data_received;
extern volatile float angular_velocity_y;
extern volatile float angular_velocity_z;
extern uint8_t received_data_packet[11];
extern float anglea;
 
 
//配置命令数组
//解锁寄存器 FF AA 69 88 B5
//Z轴角度归零FF AA 76 00 00
//设置200HZ输出 FF AA 03 0B 00
//设置波特率115200 FF AA 04 06 00
//保存 FF AA 00 00 00
//重启 FF AA 00 FF 00
uint8_t unlock_register[] = {0xFF, 0xAA, 0x69, 0x88, 0xB5};
uint8_t reset_z_axis[] = {0xFF, 0xAA, 0x76, 0x00, 0x00};
uint8_t set_output_200Hz[] = {0xFF, 0xAA, 0x03, 0x0B, 0x00};
uint8_t set_baudrate_115200[] = {0xFF, 0xAA, 0x04, 0x06, 0x00};
uint8_t save_settings[] = {0xFF, 0xAA, 0x00, 0x00, 0x00};
uint8_t restart_device[] = {0xFF, 0xAA, 0x00, 0xFF, 0x00};
 
 
 
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
		
    Usart3_SendArray(unlock_register, sizeof(unlock_register));
		delay_ms(400);
    Usart3_SendArray(reset_z_axis, sizeof(reset_z_axis));
    Usart3_SendArray(set_output_200Hz, sizeof(set_output_200Hz));
    Usart3_SendArray(set_baudrate_115200, sizeof(set_baudrate_115200));
    Usart3_SendArray(save_settings, sizeof(save_settings));
    Usart3_SendArray(restart_device, sizeof(restart_device));
	
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
    for (uint16_t i = 0; i < length; i++) {
        Usart3_SendByte(array[i]);
    }
    Usart3_SendByte(0x0D); // 发送回车符
    Usart3_SendByte(0x0A); // 发送换行符
		delay_ms(200);
	
}
 
 
void USART3_IRQHandler(void) {
    static uint8_t rx_buffer[11];
    static uint8_t rx_index = 0;
    static uint8_t state = 0; // 0: 寻找包头，1: 接收数据
 
    if (USART_GetITStatus(USART3, USART_IT_RXNE) != RESET) {
        uint8_t data = USART_ReceiveData(USART3);
 
        if (state == 0) { // 寻找包头
            if (data == 0x55) {
                rx_buffer[0] = data;
                rx_index = 1;
                state = 1;
            }
        } else if (state == 1) { // 接收数据
            rx_buffer[rx_index++] = data;
            if (rx_index == 2 && rx_buffer[1] != 0x53 && rx_buffer[1] != 0x52) { // 如果第二字节不是0x53或0x52，则继续寻找下一个0x55
                if (rx_buffer[1] == 0x55) {
                    rx_index = 1;
                } else {
                    state = 0;
                    rx_index = 0;
                }
            } else if (rx_index == 11) { // 接收到了完整的数据包
                ParseAndPrintData(rx_buffer, 11);
                rx_index = 0;
                state = 0; // 重新寻找包头
            }
        }
 
        USART_ClearITPendingBit(USART3, USART_IT_RXNE);
    }
}
 
void ParseAndPrintData(uint8_t *data, uint16_t length) {
    if (length == 11) {
        uint8_t checksum = CalculateChecksum(data, length - 1, data[1]);
        if (checksum != data[length - 1]) {
//            Usart3_Printf("Checksum error\r\n");
            return;
        }
 
        if (data[0] == 0x55 && data[1] == 0x53) {
            // 数据部分从索引 6 开始，低字节在前，高字节在后
            uint8_t yaw_l = data[6];
            uint8_t yaw_h = data[7];
            int16_t yaw = (int16_t)((yaw_h << 8) | yaw_l);
 
            // 将解析后的整数值转换为角度
            float angle = ((float)yaw / 32768.0) * 180.0;
            global_angle = angle;
            new_data_received = 1;
        } else if (data[0] == 0x55 && data[1] == 0x52) {
            // 解析角速度数据
            uint8_t wy_l = data[4];
            uint8_t wy_h = data[5];
            int16_t wy = (int16_t)((wy_h << 8) | wy_l);
            angular_velocity_y = ((float)wy / 32768.0) * 2000.0;
 
            uint8_t wz_l = data[6];
            uint8_t wz_h = data[7];
            int16_t wz = (int16_t)((wz_h << 8) | wz_l);
            angular_velocity_z = ((float)wz / 32768.0) * 2000.0;
 
            new_data_received = 1;
        }
    }
}
 
uint8_t CalculateChecksum(uint8_t *data, uint16_t length, uint8_t type) 
{
    uint8_t checksum = 0;
    for (uint16_t i = 0; i < length; i++) {
        checksum += data[i];
    }
    return checksum;
}