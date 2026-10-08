#include "sys.h"
#include "uart_5.h"
#include "stdio.h"
#include "string.h"
#include "stdarg.h"
#include "stm32f10x_tim.h"
#include "stm32f10x.h"                  // Device header
#include "uart_4.h"
#include "delay.h"
#include "OLED.h"
///////////////////////////////////////////////////////
/*
引脚占用：
UART5_TX        PC12                                                                                                                                                          
UART5_RX        PD2
扫码模块 TX 接 PD2，串口屏 RX 接 PC12；屏幕 TX 不接。
*/
///////////////////////////////////////////////////////
volatile uint8_t saoma_ready = 0;
/* Diagnostics only: raw bytes remain available in the Keil watch window. */
volatile uint8_t scanner_stage = 0;
volatile uint32_t scanner_rx_count = 0;
volatile uint32_t scanner_error_count = 0;
volatile uint32_t scanner_last_error = 0;
volatile uint8_t scanner_raw_tail[8];
volatile uint32_t scanner_poll_rx_count = 0;
volatile uint8_t scanner_pd2_level = 1;
static uint8_t scan_window[7];
static uint8_t scan_count = 0;
uint8_t saoma_data[1000];

/* UART5 RX: scanner; UART5 TX: HMI. Only the newest waiting scan is kept.
 * Queue access is serialized by UART5 IRQ or Scanner_PollReceive's IRQ mask.
 * An active frame is private to the TX ISR and is never overwritten by RX.
 */
#define HMI_SCAN_LENGTH 7U
#define HMI_COMMAND_PREFIX "page0.t0.txt=\""
#define HMI_FRAME_LENGTH ((sizeof(HMI_COMMAND_PREFIX) - 1U) + HMI_SCAN_LENGTH + 4U)
static volatile uint8_t hmi_pending_scan[HMI_SCAN_LENGTH];
static volatile uint8_t hmi_pending = 0;
static uint8_t hmi_tx_frame[HMI_FRAME_LENGTH];
static uint8_t hmi_tx_index = HMI_FRAME_LENGTH;

static void HMI_QueueScan(const uint8_t *scan)
{
    uint8_t i;
    for (i = 0; i < HMI_SCAN_LENGTH; ++i)
        hmi_pending_scan[i] = scan[i];
    hmi_pending = 1;
    USART_ITConfig(UART5, USART_IT_TXE, ENABLE);
}

/* Called only with TXE set. No waits, printf, delays or OLED work in the ISR. */
static void HMI_TransmitByte(void)
{
    static const char prefix[] = HMI_COMMAND_PREFIX;
    uint8_t i;
    uint8_t pos;

    if (hmi_tx_index == HMI_FRAME_LENGTH)
    {
        if (!hmi_pending)
        {
            USART_ITConfig(UART5, USART_IT_TXE, DISABLE);
            return;
        }
        pos = 0;
        for (i = 0; i < sizeof(prefix) - 1U; ++i)
            hmi_tx_frame[pos++] = (uint8_t)prefix[i];
        for (i = 0; i < HMI_SCAN_LENGTH; ++i)
            hmi_tx_frame[pos++] = hmi_pending_scan[i];
        hmi_tx_frame[pos++] = '"';
        hmi_tx_frame[pos++] = 0xFF;
        hmi_tx_frame[pos++] = 0xFF;
        hmi_tx_frame[pos] = 0xFF;
        hmi_pending = 0;
        hmi_tx_index = 0;
    }

    USART_SendData(UART5, hmi_tx_frame[hmi_tx_index++]);
    if ((hmi_tx_index == HMI_FRAME_LENGTH) && !hmi_pending)
        USART_ITConfig(UART5, USART_IT_TXE, DISABLE);
}



volatile uint8_t uart5_RxData;              // UART5 接收到的数据
void uart_init5()
{

    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    saoma_ready = 0;
    scanner_stage = 1;
    scanner_rx_count = 0;
    scanner_poll_rx_count = 0;
    scanner_error_count = 0;
    scanner_last_error = 0;
    scanner_pd2_level = 1;
    scan_count = 0;
    hmi_pending = 0;
    hmi_tx_index = HMI_FRAME_LENGTH;
    memset((void *)hmi_pending_scan, 0, sizeof(hmi_pending_scan));
    memset(hmi_tx_frame, 0, sizeof(hmi_tx_frame));
    memset(saoma_data, 0, sizeof(saoma_data));

    /* 开启UART5，GPIOC和GPIOD的时钟 */
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_UART5,ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOD,ENABLE);
    
    /* 复位串口5 */
    USART_DeInit(UART5);
    USART_ITConfig(UART5, USART_IT_TXE, DISABLE);

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


/* 原始扫码内容格式为 123+321；接受 CR、CRLF、DT 或无后缀。
 * 使用固定长度窗口寻找有效内容，不把模块应答和前后缀当作物料编号。
 * 屏幕接受前三位 1~6、后三位 1~3，并显示最新扫码内容。
 * 运动任务仍只使用颜色编号 1、2、3，锁定本次上电的第一组有效数据。
 */
static void Scanner_ReceiveByte(uint8_t data)
{
    uint8_t i;

    if (scan_count == sizeof(scan_window))
    {
        for (i = 1; i < sizeof(scan_window); ++i)
            scan_window[i - 1] = scan_window[i];
        --scan_count;
    }
    scan_window[scan_count++] = data;
    if (scan_count != sizeof(scan_window)) return;
    if (scan_window[3] != '+') return;
    for (i = 0; i < sizeof(scan_window); ++i)
    {
        if (i == 3) continue;
        if (scan_window[i] < '1') return;
        if (scan_window[i] > ((i < 3) ? '6' : '3')) return;
    }

    HMI_QueueScan(scan_window);

    /* Preserve the original motion data: first scan containing only 1..3. */
    if (saoma_ready) return;
    for (i = 0; i < 3; ++i)
        if (scan_window[i] > '3') return;

    memcpy(saoma_data, scan_window, sizeof(scan_window));
    saoma_data[7] = '\0';
    saoma_ready = 1;
}

/* 每个进入 UART5 的字节都会经过这里，无论它来自中断还是前台轮询。 */
static void Scanner_ProcessByte(uint8_t data)
{
    uint8_t i;
    for (i = 1; i < sizeof(scanner_raw_tail); ++i)
        scanner_raw_tail[i - 1] = scanner_raw_tail[i];
    scanner_raw_tail[sizeof(scanner_raw_tail) - 1] = data;
    Scanner_ReceiveByte(data);
}

void Scanner_WaitForData(void)
{
    /* 1=initialized, 2=waiting, 3=valid scan, 4=sending, 5=send returned. */
    scanner_stage = 2;
    while (!saoma_ready)
        delay_ms(1);
    scanner_stage = 3;
}

void UART5_IRQHandler(void)
{
    uint32_t status = UART5->SR;
    if (status & (USART_FLAG_RXNE | USART_FLAG_ORE |
                  USART_FLAG_FE | USART_FLAG_NE | USART_FLAG_PE))
    {
        /* 先读 SR 再读 DR，清除接收/错误状态，避免反复进入中断。 */
        uart5_RxData = (uint8_t)UART5->DR;
        if (status & USART_FLAG_RXNE)
        {
            ++scanner_rx_count;
            if (!(status & (USART_FLAG_ORE | USART_FLAG_FE |
                            USART_FLAG_NE | USART_FLAG_PE)))
                Scanner_ProcessByte(uart5_RxData);
        }
        if (status & (USART_FLAG_ORE | USART_FLAG_FE |
                      USART_FLAG_NE | USART_FLAG_PE))
        {
            ++scanner_error_count;
            scanner_last_error = status & (USART_FLAG_ORE | USART_FLAG_FE |
                                            USART_FLAG_NE | USART_FLAG_PE);
            scan_count = 0;
        }
    }
    if (USART_GetITStatus(UART5, USART_IT_TXE) != RESET)
        HMI_TransmitByte();
}

/* 前台接收通道：只有当 RXNE 已经置位、中断却没有来处理时才会收到字节。
 * 中断正常工作时这里永远读不到东西，因此它能把
 * “数据没到 PD2”和“UART5 收到了但中断没处理”直接分开。
 * 同时它也作为后备通道，中断失效时仍能完成扫码。
 */
static void Scanner_PollReceive(void)
{
    uint32_t status;
    uint32_t mask;
    uint8_t data;

    mask = __get_PRIMASK();
    __disable_irq();
    status = UART5->SR;
    if (status & (USART_FLAG_RXNE | USART_FLAG_ORE |
                  USART_FLAG_FE | USART_FLAG_NE | USART_FLAG_PE))
    {
        data = (uint8_t)UART5->DR;
        if (status & USART_FLAG_RXNE)
        {
            ++scanner_poll_rx_count;
            if (!(status & (USART_FLAG_ORE | USART_FLAG_FE |
                            USART_FLAG_NE | USART_FLAG_PE)))
                Scanner_ProcessByte(data);
        }
        if (status & (USART_FLAG_ORE | USART_FLAG_FE |
                      USART_FLAG_NE | USART_FLAG_PE))
        {
            ++scanner_error_count;
            scanner_last_error = status & (USART_FLAG_ORE | USART_FLAG_FE |
                                            USART_FLAG_NE | USART_FLAG_PE);
            scan_count = 0;
        }
    }
    /* PD2 是 UART5_RX，空闲时应为高电平。 */
    scanner_pd2_level = (GPIOD->IDR & GPIO_Pin_2) ? 1 : 0;
    __set_PRIMASK(mask);
}

/* 屏幕上的结论行，直接把下一步该查哪里写出来。 */
static const char *Scanner_ResultText(void)
{
    if (saoma_ready)
        return (scanner_poll_rx_count != 0) ? "POLL OK" : "SCAN OK";
    if ((scanner_rx_count != 0) || (scanner_poll_rx_count != 0))
        return (scanner_rx_count == 0) ? "IRQ DEAD" : "DATA ERR";
    if (scanner_error_count != 0)
    {
        if (scanner_last_error & USART_FLAG_ORE) return "OVERRUN";
        if (scanner_last_error & USART_FLAG_FE) return "FRAME ERR";
        if (scanner_last_error & USART_FLAG_PE) return "PARITY";
        if (scanner_last_error & USART_FLAG_NE) return "NOISE ERR";
        return "UART ERR";
    }
    if (!scanner_pd2_level) return "PD2 LOW";
    return "NO DATA";
}

/* Foreground-only display; UART interrupts only update diagnostics. */
static uint8_t scanner_oled_enabled = 0;
static uint32_t scanner_oled_elapsed = 0;
static char scanner_oled_previous[4][17];
volatile uint8_t scanner_camera_tx_valid = 0;
volatile uint8_t scanner_camera_tx_mode = 0;

static void Scanner_DisplayRefresh(void)
{
    char rows[4][17];
    char text[24];
    uint8_t line, col;

    snprintf(text, sizeof(text), "IRQ:%lu P:%lu",
             (unsigned long)(scanner_rx_count % 100000),
             (unsigned long)(scanner_poll_rx_count % 10000));
    snprintf(rows[0], sizeof(rows[0]), "%-16.16s", text);

    if (scanner_camera_tx_valid)
        snprintf(text, sizeof(text), "ERR:%lu TX:%02X",
                 (unsigned long)(scanner_error_count % 10000),
                 (unsigned)scanner_camera_tx_mode);
    else
        snprintf(text, sizeof(text), "ERR:%lu PD2:%u",
                 (unsigned long)(scanner_error_count % 10000),
                 (unsigned)scanner_pd2_level);
    snprintf(rows[1], sizeof(rows[1]), "%-16.16s", text);

    snprintf(text, sizeof(text), "DATA:%s",
             saoma_ready ? (char *)saoma_data : "-------");
    snprintf(rows[2], sizeof(rows[2]), "%-16.16s", text);

    snprintf(text, sizeof(text), "RESULT:%s", Scanner_ResultText());
    snprintf(rows[3], sizeof(rows[3]), "%-16.16s", text);

    for (line = 0; line < 4; ++line)
        for (col = 0; col < 16; ++col)
            if (rows[line][col] != scanner_oled_previous[line][col])
            {
                OLED_ShowChar(line + 1, col + 1, rows[line][col]);
                scanner_oled_previous[line][col] = rows[line][col];
            }
}

void Scanner_DisplayInit(void)
{
    memset(scanner_oled_previous, 0, sizeof(scanner_oled_previous));
    scanner_oled_elapsed = 0;
    scanner_oled_enabled = 1;
    Scanner_DisplayRefresh();
}

void Scanner_DisplayPoll(uint32_t elapsed_ms)
{
    if ((SCB->ICSR & 0x1FFU) != 0) return;   /* 中断里不轮询、不刷屏 */
    Scanner_PollReceive();
    if (!scanner_oled_enabled) return;
    if (elapsed_ms >= 200) scanner_oled_elapsed = 200;
    else scanner_oled_elapsed += elapsed_ms;
    if (scanner_oled_elapsed < 200) return;
    scanner_oled_elapsed = 0;
    Scanner_DisplayRefresh();
}
