#include "board.h"
#include "delay.h"
#include "usart.h"
#include "Emm_V5.h"
#include "car.h"
#include "task.h"
#include "PWM.h"
#include <string.h>
#include "uart_3.h"
#include "uart_5.h"
#include "PID.h" 
#include "OLED.h"
#include "uart_4.h"
#include "usart2.h"
#include "openmv.h"
#include "stm32f10x.h"
#include "sys_tick.h"
#include "wuliao.h"
#include "usart2.h"
#include "fashion_star_uart_servo.h"
#include "stm32f10x.h"

volatile float global_angle = 0.0;
volatile float angular_velocity_y = 0.0;
volatile float angular_velocity_z = 0.0;
volatile uint8_t new_data_received = 0;
volatile float angle_offset = 0.0;


extern float anglea;

/* 2文件通信抓取测试：1=原地测试，0=恢复下面原有行走路线。 */
#define CAMERA_GRAB_TEST_ENABLE 1
/* 主控发送的识别模式：1红、2黄、3蓝、4绿、5黑、6浅蓝；只改这一行。 */
#define CAMERA_TEST_MODE 1

#if CAMERA_GRAB_TEST_ENABLE
#if CAMERA_TEST_MODE < 1 || CAMERA_TEST_MODE > 6
#error CAMERA_TEST_MODE_must_be_1_to_6_for_material_grab
#endif

/* 调试观察：0=等待识别，1=正在执行抓放动作，2=动作函数已返回。 */
volatile uint8_t camera_grab_test_state = 0;

static void Camera_GrabTest_Run(void)
{
    uint8_t id, received;
    uint32_t now, last_send_ms;
    MaixCAM_Target target;

    /* 测通信时底盘保持原地，使用原有爪子准备动作。 */
    for (id = 1; id <= 4; ++id)
    {
        Emm_V5_Stop_Now(id, 0);
        delay_ms(10);
    }
    MaixCAM_SetMode(MAIXCAM_MODE_OFF);
    zhuazikai();
    zhuanpan0();
    delay_ms(300);

    /* 主控发送位置：默认发一个原始字节 0x01，让摄像头识别红色。 */
    MaixCAM_SetMode(CAMERA_TEST_MODE);
    last_send_ms = MaixCAM_NowMs();

    for (;;)
    {
        received = MaixCAM_GetTarget(&target);
        now = MaixCAM_NowMs();
        if (received && MaixCAM_TargetFresh(&target, now, MAIXCAM_LINK_TIMEOUT_MS))
        {
            /* found=1且数据未过期即触发；本测试不按dx/dy移动底盘。 */
            camera_grab_test_state = 1;
            MaixCAM_SetMode(MAIXCAM_MODE_OFF);
            wuliao1();  /* 原动作：抓取物料，放到1号托盘，再转回前方。 */
            camera_grab_test_state = 2;
            return;    /* 每次复位只抓一次，避免摄像头重复发包导致连抓。 */
        }

        /* 摄像头晚启动/重启时，断流期间每2秒补发模式；正常收包不重发。 */
        if ((uint32_t)(now - last_send_ms) >= 2000U &&
            (!received || (uint32_t)(now - target.received_ms) > MAIXCAM_LINK_TIMEOUT_MS))
        {
            MaixCAM_SetMode(CAMERA_TEST_MODE);
            last_send_ms = MaixCAM_NowMs();
        }
        delay_ms(10);
    }
}
#endif

/**********************************************************
***	Emm_V5.0步进闭环控制例程
***	编写作者：ZHANGDATOU
***	技术支持：张大头闭环伺服
***	淘宝店铺：https://zhangdatou.taobao.com
***	CSDN博客：http s://blog.csdn.net/zhangdatou666
***	qq交流群：262438510
**********************************************************/

/**
	*	@brief		MAIN函数
	*	@param		无
	*	@retval		无
	*/
int main(void)
{
/**********************************************************
***	初始化板载外设
**********************************************************/
	uart_init4();
	board_init();
	OLED_Init();
	Usart3_Init();
	PWM_Init();
	uart_init5();
	Usart_Init2();
	SysTick_Init();
	
	FSUS_ServoAngleReset(&usart2,0);     //重置多圈圈数
//	FSUS_DampingMode(&usart2,0,1000);     //设置阻尼模式


//		/*串口5配置*/

//	 delay_ms(20);//等待就绪

///**********************************************************
//***	上电延时2秒等待Emm_V5.0闭环初始化完毕
////**********************************************************/	
	delay_ms(1000);

///**********************************************************
//***	位置模式：方向CW，速度1000RPM，加速度0（不使用加减速直接启动），脉冲数3200（16细分下发送3200个脉冲电机转一圈），相对运动
//**********************************************************/	
//	
//	
//	Emm_V5_Pos_Control(5, 0, 150, 200, 6800, 0, 0);
//	
//	vision('6');
//		zhou(guiwei); 
//

//int speed_hua=300;
//int speed_hua_jia=500;

//		fnwuliao2();
//		delay_ms(400);
//		
//	zhou(zhua2);	        //轴转动，准备将物料1放进托盘
//		
//		Emm_V5_Pos_Control(5, 1, speed_hua, speed_hua_jia, 1800, 0, 0);    //升降台下降
//		
//	delay_ms(150);
//	
//		jiazi(fang);     //将物料1放进托盘
//		
//		Emm_V5_Pos_Control(5, 0, speed_hua, speed_hua_jia, 1800, 0, 0);      //升降台上升
//	
//	delay_ms(150);
//		
//		zhou(guiwei);    //夹子正对物料

//	Emm_V5_Pos_Control(5, 1, 500, 500, 1600*4, 0, 0);
//	delay_ms(150);
//	jiazi(zhua);
//	delay_ms(150);	
//	Emm_V5_Pos_Control(5, 0, 500, 500, 1600*4, 0, 0);       //升降台上升	
//	delay_ms(200);		
//	zhou(zhua3);
//	Emm_V5_Pos_Control(5, 1, 500, 500, 1800*4, 0, 0);    //升降台下降	
//	delay_ms(200);
//	jiazi(fang);     //放物料
//	Emm_V5_Pos_Control(5, 0, 500, 500, 1800*4, 0, 0);      //升降台上升
//	
//	delay_ms(200);
//		
//	zhou(guiwei);    //夹子正对物料
#if CAMERA_GRAB_TEST_ENABLE
    Camera_GrabTest_Run();
#else
	yuanliao();
	cujiagong();
	zancunqu();
	yuanliao2();
	cujiagong2();
	zancunqu2();
	qidian();
#endif

//	
///**********************************************************
//***	等待返回命令，命令数据缓存在数组rxCmd上，长度为rxCount
//**********************************************************/	
//	while(rxFrameFlag == false); rxFrameFlag = false;

/**********************************************************
***	WHILE循环
**********************************************************/	
	while(1)
	{
			
	}
}




void TIM2_IRQHandler(void)
{
	if (TIM_GetITStatus(TIM2, TIM_IT_Update) == SET)
	{
		
//		x1 = pid3(x,110);
//		  	y1 = pid2(y,90);
//		OLED_ShowSignedNum( 1,  1,x1, 4 );
//		OLED_ShowSignedNum( 2,  1,  y1,  4);
//						Usart_SendByte4(UART4, x);
//				Usart_SendByte4(UART4, y);
//		if(rxFrameFlag == true)
//		{
//					Usart_SendByte4(UART4, rxCmd[1]);
//			Usart_SendByte4(UART4, rxCmd[2]);
//			Usart_SendByte4(UART4, rxCmd[3]);
//		}
//		rxFrameFlag = false;
    MaixCAM_Tick20ms();
    anglea = angle(global_angle - angle_offset, 0, 0);//角度环目标角度
//				Usart_SendByte4(UART4, uart4_RxData);
		TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
	}
}
