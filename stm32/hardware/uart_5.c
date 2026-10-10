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
#include "red_align_test.h"
#include "gyro_debug.h"
#include "camera_debug.h"
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
static uint8_t scan_window[15];
static uint8_t scan_count = 0;
uint8_t saoma_data[1000];

/* UART5 RX receives scans; UART5 TX updates the HMI without blocking RX.
 * RX and polling serialize queue access; the active TX frame stays private.
 */
#define HMI_SCAN_LENGTH 15U
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


/* Four-group scan: color1+ring1+color2+ring2, e.g. 123+321+132+213.
 * HMI displays valid frames with colors 1..6 and rings 1..3.
 * Motion currently locks only four 1..3 permutations, matching the
 * implemented three-color pickup and ring-placement routines.
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
    if (scan_window[3] != '+' || scan_window[7] != '+' ||
        scan_window[11] != '+') return;
    for (i = 0; i < sizeof(scan_window); ++i)
    {
        if (i == 3 || i == 7 || i == 11) continue;
        if (scan_window[i] < '1') return;
        if (scan_window[i] > (((i < 3) || (i >= 8 && i < 11)) ? '6' : '3')) return;
    }

    HMI_QueueScan(scan_window);
    if (saoma_ready) return;
    /* Current motion routines support colors 1..3 and one item per ring. */
    for (i = 0; i < 3; ++i)
        if (scan_window[i] > '3' || scan_window[8 + i] > '3') return;
    for (i = 0; i < 4; ++i)
    {
        uint8_t base = (uint8_t)(4U * i);
        if (scan_window[base] == scan_window[base + 1] ||
            scan_window[base] == scan_window[base + 2] ||
            scan_window[base + 1] == scan_window[base + 2]) return;
    }

    memcpy(saoma_data, scan_window, sizeof(scan_window));
    saoma_data[sizeof(scan_window)] = '\0';
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
#if !GYRO_OLED_DEBUG_ENABLE && !CAMERA_OLED_DEBUG_ENABLE
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

#endif

/* Foreground-only display; UART interrupts only update diagnostics. */
static uint8_t scanner_oled_enabled = 0;
#if !GYRO_OLED_DEBUG_ENABLE && !CAMERA_OLED_DEBUG_ENABLE
static uint32_t scanner_oled_elapsed = 0;
#endif
static char scanner_oled_previous[4][17];
volatile uint8_t scanner_camera_tx_valid = 0;
volatile uint8_t scanner_camera_tx_mode = 0;


#if GYRO_OLED_DEBUG_ENABLE || CAMERA_OLED_DEBUG_ENABLE
extern volatile float global_angle;
extern float anglea;
#define GYRO_DWT_CTRL   (*(volatile uint32_t *)0xE0001000UL)
#define GYRO_DWT_CYCCNT (*(volatile uint32_t *)0xE0001004UL)
static uint32_t gyro_oled_cycles;
#if GYRO_OLED_DEBUG_ENABLE && !CAMERA_OLED_DEBUG_ENABLE
static uint32_t gyro_oled_previous_rx;

static void Gyro_FormatRows(char rows[4][17])
{
    float yaw;
    int control, target, mode, yaw10;
    unsigned magnitude;
    uint8_t running, line, col;
    uint32_t packets, mask;
    const char *state;

    mask = __get_PRIMASK();
    __disable_irq();
    yaw = global_angle;
    control = (int)anglea;
    packets = gyro_yaw_frame_count;
    target = car_debug_target;
    mode = car_debug_mode;
    running = car_debug_active;
    __set_PRIMASK(mask);

    if (packets)
    {
        yaw10 = (int)(yaw * 10.0f + (yaw >= 0.0f ? 0.5f : -0.5f));
        magnitude = (unsigned)(yaw10 < 0 ? -yaw10 : yaw10);
        snprintf(rows[0], 17, "YAW:%c%03u.%u", yaw10 < 0 ? '-' : '+',
                 magnitude / 10U, magnitude % 10U);
    }
    else
        snprintf(rows[0], 17, "YAW:---.-");

    if (running && mode == 0)
        snprintf(rows[1], 17, "CTL:%+04d T:%+04d", control, target);
    else
        snprintf(rows[1], 17, "CTL:%+04d T:----", control);
    if (mode >= 0)
        snprintf(rows[2], 17, "CAR:%02d RUN:%u", mode, (unsigned)running);
    else
        snprintf(rows[2], 17, "CAR:-- RUN:0");

    state = !packets ? "NO DATA" :
            (packets != gyro_oled_previous_rx ? "UPDATE" : "HOLD");
    snprintf(rows[3], 17, "RX:%05lu %s", (unsigned long)(packets % 100000UL), state);
    gyro_oled_previous_rx = packets;
    for (line = 0; line < 4; ++line)
    {
        for (col = (uint8_t)strlen(rows[line]); col < 16; ++col)
            rows[line][col] = ' ';
        rows[line][16] = '\0';
    }
}
#endif


#endif /* Common DWT display clock. */

#if CAMERA_OLED_DEBUG_ENABLE
static uint32_t camera_oled_heartbeat;
static void CameraDebug_FormatRows(char rows[4][17])
{
    uint32_t mask, packets, bytes, bad, uart_errors, last_ms, now, age;
    uint8_t mode, found, line, col;
    int16_t dx, dy;
    mask = __get_PRIMASK();
    __disable_irq();
    mode = maixcam_mode;
    found = maixcam_found;
    dx = maixcam_dx;
    dy = maixcam_dy;
    packets = camera_debug_frames;
    bytes = camera_debug_rx_bytes;
    bad = camera_debug_bad_frames;
    uart_errors = camera_debug_uart_errors;
    last_ms = camera_debug_last_frame_ms;
    now = CameraDebug_NowMs();
    __set_PRIMASK(mask);
    ++camera_oled_heartbeat;
    snprintf(rows[0], 17, "M%u F%u RX%06lu", (unsigned)mode,
             (unsigned)found, (unsigned long)(packets % 1000000UL));
    if (packets)
    {
        snprintf(rows[1], 17, "X%+06d Y%+06d", (int)dx, (int)dy);
        age = (uint32_t)(now - last_ms);
        if (age > 99999UL) age = 99999UL;
        snprintf(rows[2], 17, "A%05lu B%08lu", (unsigned long)age,
                 (unsigned long)(bytes % 100000000UL));
    }
    else
    {
        snprintf(rows[1], 17, "X------ Y------");
        snprintf(rows[2], 17, "A----- B%08lu",
                 (unsigned long)(bytes % 100000000UL));
    }
    snprintf(rows[3], 17, "C%03lu U%03lu H%04lu",
             (unsigned long)(bad % 1000UL),
             (unsigned long)(uart_errors % 1000UL),
             (unsigned long)(camera_oled_heartbeat % 10000UL));
    for (line = 0; line < 4; ++line)
    {
        for (col = (uint8_t)strlen(rows[line]); col < 16; ++col)
            rows[line][col] = ' ';
        rows[line][16] = '\0';
    }
}
#endif

static void Scanner_DisplayRefresh(void)
{
    char rows[4][17];
    uint8_t line, col;
#if !GYRO_OLED_DEBUG_ENABLE && !CAMERA_OLED_DEBUG_ENABLE
    char text[24];
    uint8_t camera_found, camera_mode;
    int16_t camera_dx, camera_dy;
    uint32_t mask;
#endif

#if CAMERA_OLED_DEBUG_ENABLE
    CameraDebug_FormatRows(rows);
#elif GYRO_OLED_DEBUG_ENABLE
    Gyro_FormatRows(rows);
#else
    if (RedAlignTest_Display()) return;
    if (scanner_camera_tx_valid)
    {
        /* Snapshot one camera frame. Task code may clear x/y between frames. */
        mask = __get_PRIMASK();
        __disable_irq();
        camera_found = maixcam_found;
        camera_mode = maixcam_mode;
        camera_dx = maixcam_dx;
        camera_dy = maixcam_dy;
        __set_PRIMASK(mask);
        snprintf(text, sizeof(text), "MODE:%u FOUND:%u",
                 (unsigned)camera_mode, (unsigned)camera_found);
        snprintf(rows[0], sizeof(rows[0]), "%-16.16s", text);
        if (camera_found && camera_dx >= -160 && camera_dx <= 159 &&
            camera_dy >= -120 && camera_dy <= 119)
        {
            snprintf(rows[1], sizeof(rows[1]), "DX:%+04d DY:%+04d ",
                     (int)camera_dx, (int)camera_dy);
            /* Same alignment reference as Camera_ReceiveByte: 122-x, 49-y. */
            snprintf(rows[2], sizeof(rows[2]), "EX:%+04d EY:%+04d ",
                     -38 - (int)camera_dx, -71 - (int)camera_dy);
        }
        else
        {
            snprintf(rows[1], sizeof(rows[1]), "DX:---- DY:---- ");
            snprintf(rows[2], sizeof(rows[2]), "EX:---- EY:---- ");
        }
        snprintf(rows[3], sizeof(rows[3]), "AIM:122,049     ");
        /* Each row must contain 16 printable characters. */
        for (line = 1; line < 4; ++line)
        {
            for (col = (uint8_t)strlen(rows[line]); col < 16; ++col)
                rows[line][col] = ' ';
            rows[line][16] = '\0';
        }
    }
    else
    {
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
    }

#endif

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
#if !GYRO_OLED_DEBUG_ENABLE && !CAMERA_OLED_DEBUG_ENABLE
    scanner_oled_elapsed = 0;
#endif
    scanner_oled_enabled = 1;
#if GYRO_OLED_DEBUG_ENABLE || CAMERA_OLED_DEBUG_ENABLE
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    GYRO_DWT_CTRL |= 1UL; /* Share the counter; never reset the vision deadline clock. */
    gyro_oled_cycles = GYRO_DWT_CYCCNT;
#if GYRO_OLED_DEBUG_ENABLE && !CAMERA_OLED_DEBUG_ENABLE
    gyro_oled_previous_rx = 0;
#endif
#endif
    Scanner_DisplayRefresh();
}

void Scanner_DisplayPoll(uint32_t elapsed_ms)
{
    if ((SCB->ICSR & 0x1FFU) != 0) return;   /* 中断里不轮询、不刷屏 */
    Scanner_PollReceive();
    if (!scanner_oled_enabled) return;
#if GYRO_OLED_DEBUG_ENABLE || CAMERA_OLED_DEBUG_ENABLE
    (void)elapsed_ms;
    Gyro_DisplayPoll();
#else
    if (elapsed_ms >= 200) scanner_oled_elapsed = 200;
    else scanner_oled_elapsed += elapsed_ms;
    if (scanner_oled_elapsed < 200) return;
    scanner_oled_elapsed = 0;
    Scanner_DisplayRefresh();
#endif
}


void Gyro_DisplayPoll(void)
{
#if GYRO_OLED_DEBUG_ENABLE || CAMERA_OLED_DEBUG_ENABLE
    uint32_t now;
    if ((SCB->ICSR & 0x1FFU) != 0 || !scanner_oled_enabled) return;
    now = GYRO_DWT_CYCCNT;
    if ((uint32_t)(now - gyro_oled_cycles) < (SystemCoreClock / 5UL)) return;
    gyro_oled_cycles = now;
    Scanner_DisplayRefresh();
#endif
}
