#include "uart_4.h"

volatile uint8_t maixcam_found = 0;
volatile int16_t maixcam_dx = 0, maixcam_dy = 0;
volatile int32_t x = 0, y = 0;
volatile int32_t error_x = 0, error_y = 0;
volatile uint8_t maixcam_mode = MAIXCAM_MODE_OFF;
volatile uint32_t maixcam_uart_errors = 0;

static volatile uint32_t camera_time_ms = 0;
static volatile uint8_t target_received = 0;
static MaixCAM_Parser parser;
static volatile MaixCAM_Target latest;

void MaixCAM_Tick20ms(void) { camera_time_ms += 20U; }
uint32_t MaixCAM_NowMs(void) { return camera_time_ms; }

void MaixCAM_ResetTarget(void)
{
    uint32_t mask = __get_PRIMASK();
    __disable_irq();
    parser.count = 0;
    target_received = 0;
    latest.found = 0;
    latest.dx = latest.dy = 0;
    maixcam_found = 0;
    maixcam_dx = maixcam_dy = 0;
    x = y = error_x = error_y = 0;
    __set_PRIMASK(mask);
}

uint8_t MaixCAM_GetTarget(MaixCAM_Target *target)
{
    uint8_t received;
    uint32_t mask = __get_PRIMASK();
    __disable_irq();
    *target = latest;
    received = target_received;
    __set_PRIMASK(mask);
    return received;
}

uint8_t MaixCAM_TargetFound(void)
{
    MaixCAM_Target target;
    return (uint8_t)(MaixCAM_GetTarget(&target) &&
        MaixCAM_TargetFresh(&target, MaixCAM_NowMs(), MAIXCAM_LINK_TIMEOUT_MS));
}

void Usart_SendByte4(USART_TypeDef *port, uint8_t byte)
{
    USART_SendData(port, byte);
    while (USART_GetFlagStatus(port, USART_FLAG_TXE) == RESET) {}
}

uint8_t MaixCAM_SetMode(uint8_t mode)
{
    if (mode > MAIXCAM_MODE_RING)
        return 0;
    MaixCAM_ResetTarget();
    maixcam_mode = mode;
    /* 原始单字节 00..07；不发送 ASCII、换行或额外帧头帧尾。 */
    Usart_SendByte4(UART4, mode);
    return 1;
}

uint8_t MaixCAM_ColorFromAscii(uint8_t character)
{
    if (character >= '1' && character <= '6')
        return (uint8_t)(character - '0');
    return MAIXCAM_MODE_OFF;
}

void uart_init4(void)
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
    MaixCAM_ResetTarget();
    USART_Cmd(UART4, ENABLE);                    //使能串口4
//		Usart_SendString4(UART4, "task");


}


void UART4_IRQHandler(void)
{
    uint32_t status = UART4->SR;
    uint8_t byte;
    MaixCAM_Target decoded;
    if (status & (USART_FLAG_PE | USART_FLAG_FE | USART_FLAG_NE | USART_FLAG_ORE))
    {
        (void)UART4->DR;
        ++maixcam_uart_errors;
        MaixCAM_ResetTarget();
        return;
    }
    if (!(status & USART_FLAG_RXNE))
        return;
    byte = (uint8_t)UART4->DR;
    decoded = latest;
    if (MaixCAM_ParseByte(&parser, byte, camera_time_ms, &decoded))
    {
        latest = decoded;
        target_received = 1;
        maixcam_found = latest.found;
        maixcam_dx = latest.dx;
        maixcam_dy = latest.dy;
        if (latest.found)
        {
            x = 160L + latest.dx;
            y = 120L + latest.dy;
            /* 新协议是 target-center；原车控制误差采用 center-target。 */
            error_x = -(int32_t)latest.dx;
            error_y = -(int32_t)latest.dy;
        }
        else
        {
            x = y = error_x = error_y = 0;
        }
    }
}
