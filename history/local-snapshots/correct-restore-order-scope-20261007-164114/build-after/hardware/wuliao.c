#include "stm32f10x.h"
#include "sys_tick.h"
#include "fashion_star_uart_servo.h"
#include "usart2.h"
#include "Emm_V5.h"
#include "usart.h"
#include "board.h"
#include "fifo.h"
#include "delay.h"
#include "car.h"
#include "red_align_test.h"
#include "openmv.h"
#include "uart_4.h"
#include "uart_5.h"
#include "OLED.h"
#include <stdio.h>


//轴ID：0（紫色）     朝向物料：160；   1号盘：305；   2号盘：-20；   3号盘：17；
//爪子ID：1（绿色）   抓物料：165；  开爪子：120；  松料：150；
//参数：串口，舵机ID，角度，速度，加速时间，减速时间，舵机执行功率（0），1
//步进电机 向上运动：0    向下运动：1
extern __IO uint8_t rxCmd[FIFO_SIZE];


int zhua1=305;
int zhua2=-20;
int zhua3=17;
int fang1=305;
int fang3=17;
int fang2=-20;

int zhua=165;
int fang=150;
int guiwei=162;

int speed_hua=500;
int speed_hua_jia=200;

int speed_hua_fang=400;
int speed_hua_jia_fang=500;

void wait(){
	while(1)
		{
			if (rxCmd[1] == 0xFD && rxCmd[2] == 0x9F && rxCmd [3] == 0x6B)
			{	
				rxCmd[1] = 0x00;
				rxCmd[2] = 0x00;
				rxCmd[3] = 0x00;
				delay_ms(20);
				break;
		}
		}
}

void zhou(int angle)
{
    FSUS_SetServoAngleMTurnByVelocity(&usart2,0,angle,450,300,300,0,0);
	delay_ms(500);
}

void jiazi(int angle)
{
		FSUS_SetServoAngleMTurnByVelocity(&usart2,1,angle,750,150,150,0,0);
	delay_ms(100);
}


void  ceshi(void)
{
	jiazi(fang);
	zhou(guiwei);
	delay_ms(500);
	Emm_V5_Pos_Control(5, 1, speed_hua, speed_hua_jia, 5400, 0, 0);
	delay_ms(650);
	jiazi(zhua);
	delay_ms(500);
	Emm_V5_Pos_Control(5, 0, speed_hua, speed_hua_jia, 5400, 0, 0);
	delay_ms(2000);
	zhou(zhua1);
	delay_ms(100);
	Emm_V5_Pos_Control(5, 1, speed_hua, speed_hua_jia, 4550, 0, 0);
	delay_ms(650);
	jiazi(fang);
	delay_ms(500);
	Emm_V5_Pos_Control(5, 0, speed_hua, speed_hua_jia, 4550, 0, 0);
	for(; ;);
	delay_ms(100);
	
}

void wuliao1(void)
{
	//准备抓物料之前，爪子升到最高处并且张开120°，朝向物料
	
//	zhou(guiwei);            //夹子正对物料
	
	Emm_V5_Pos_Control(5, 1, speed_hua, speed_hua_jia, 5400, 0, 0);        //升降台下降
	//转盘高度降低增加300
	delay_ms(650);
		
		jiazi(zhua);           //抓物料
	
		delay_ms(150);
		
		Emm_V5_Pos_Control(5, 0, speed_hua, speed_hua_jia, 5400, 0, 0);       //升降台上升
		
	delay_ms(200);
		
	zhou(zhua1);	        //轴转动，准备放物料1
		
		Emm_V5_Pos_Control(5, 1, speed_hua, speed_hua_jia, 4550, 0, 0);    //升降台下降
		
	delay_ms(650);
	
		jiazi(fang);     //放物料
		
		Emm_V5_Pos_Control(5, 0, speed_hua, speed_hua_jia, 4550, 0, 0);      //升降台上升
	
	delay_ms(400);
		
		zhou(guiwei);    //夹子正对物料
		
}


void wuliao2(void)
{
	//准备抓物料之前，爪子升到最高处并且张开120°，朝向物料
	
//	zhou(guiwei);            //夹子正对物料
	
	Emm_V5_Pos_Control(5, 1, speed_hua, speed_hua_jia, 5400, 0, 0);        //升降台下降
	
	delay_ms(650);
		
		jiazi(zhua);           //抓物料
	
		delay_ms(200);
		
		Emm_V5_Pos_Control(5, 0, speed_hua, speed_hua_jia, 5400, 0, 0);       //升降台上升
		
	delay_ms(250);
		
	zhou(zhua2);	        //轴转动，准备放物料2
	delay_ms(350);
		
		Emm_V5_Pos_Control(5, 1, speed_hua, speed_hua_jia, 4550, 0, 0);    //升降台下降
		
	delay_ms(650);
	
		jiazi(fang);     //放物料
		
		Emm_V5_Pos_Control(5, 0, speed_hua, speed_hua_jia, 4550, 0, 0);      //升降台上升
	
	delay_ms(250);
		
		zhou(guiwei);    //夹子正对物料

}


void wuliao3(void)
{
	//准备抓物料之前，爪子升到最高处并且张开120°，朝向物料
	
//	zhou(guiwei);            //夹子正对物料
	
	Emm_V5_Pos_Control(5, 1, speed_hua, speed_hua_jia, 5400, 0, 0);        //升降台下降
	
	delay_ms(650);
		
		jiazi(zhua);           //抓物料
	
		delay_ms(150);
		
		Emm_V5_Pos_Control(5, 0, speed_hua, speed_hua_jia, 5400, 0, 0);       //升降台上升
		
	delay_ms(250);
		
	zhou(zhua3);	        //轴转动，准备放物料3
		
		Emm_V5_Pos_Control(5, 1, speed_hua, speed_hua_jia, 4550, 0, 0);    //升降台下降
		
	delay_ms(650);
	
		jiazi(fang);     //放物料
		
		Emm_V5_Pos_Control(5, 0, speed_hua, speed_hua_jia, 4550, 0, 0);      //升降台上升
	
	delay_ms(200);
		
//		zhou(guiwei);    //夹子正对物料

}


void fwuliao1(void)
{
	Emm_V5_Pos_Control(5, 0, speed_hua, speed_hua_jia, 8000, 0, 0);
	delay_ms(650);
	
	jiazi(fang);
	zhou(fang1);             //转至物料1位置
	delay_ms(50);
	
	Emm_V5_Pos_Control(5, 1, speed_hua_fang, speed_hua_jia_fang, 4550,0,0);    //升降台降
	delay_ms(650);
	
	
	jiazi(zhua);      //抓物料1
	Emm_V5_Pos_Control(5, 0, speed_hua_fang, speed_hua_jia_fang, 4550,0,0);    //升降台升
	delay_ms(650);
	
	zhou(guiwei);      //转回0位置
		
	Emm_V5_Pos_Control(5, 1, speed_hua_fang, speed_hua_jia_fang, 13310,0,0);    //升降台降放地上
	
	delay_ms(650);
		
	jiazi(fang);      //放物料1

	Emm_V5_Pos_Control(5, 0, speed_hua, speed_hua_jia, 5310, 0, 0);    //升降台升
	
	delay_ms(250);
	
}	


void fwuliao2(void)
{
	Emm_V5_Pos_Control(5, 0, speed_hua, speed_hua_jia, 8000, 0, 0);	//升到最高处
	delay_ms(650);
	jiazi(fang);
	zhou(fang2);              //转至物料2位置
	delay_ms(50);
	
	Emm_V5_Pos_Control(5, 1, speed_hua, speed_hua_jia, 4550, 0, 0);        //升降台降
	
	delay_ms(650);
		 
		jiazi(zhua);         //抓物料2
		
	Emm_V5_Pos_Control(5, 0, speed_hua, speed_hua_jia, 4550, 0, 0);            //升降台升
	
	delay_ms(650);
	
		zhou(guiwei);       //转回0位置
		
	Emm_V5_Pos_Control(5, 1, speed_hua_fang, speed_hua_jia_fang, 13310, 0, 0);           //升降台降放地上
	
	delay_ms(650);
		
	jiazi(fang);        //放物料2

	Emm_V5_Pos_Control(5, 0, speed_hua, speed_hua_jia, 5310, 0, 0);     //升降台升
	
	delay_ms(250);
	
}	


void fwuliao3(void)
{
	
	Emm_V5_Pos_Control(5, 0, speed_hua, speed_hua_jia, 8000, 0, 0);
	delay_ms(650);
	jiazi(fang);
	zhou(fang3);           //转至物料3位置

	
	Emm_V5_Pos_Control(5, 1, speed_hua, speed_hua_jia, 4550, 0, 0);        //升降台降
	
	delay_ms(650);
		
		jiazi(zhua);                //抓物料2
		
	Emm_V5_Pos_Control(5, 0, speed_hua, speed_hua_jia, 4550, 0, 0);             //升降台升
	
	delay_ms(650);
	
		zhou(guiwei);           //转回0位置
		
	Emm_V5_Pos_Control(5, 1, speed_hua_fang, speed_hua_jia_fang, 13310, 0, 0);         //升降台降
	
	delay_ms(650);
		
	jiazi(fang);        //放物料3

	Emm_V5_Pos_Control(5, 0, speed_hua, speed_hua_jia, 13310, 0, 0);        //升降台升
	
	delay_ms(250);
	
}	


void fnwuliao1(void)
{
	
	Emm_V5_Pos_Control(5, 1, speed_hua, speed_hua_jia, 6200*4, 0, 0);        //升降台下降
	
	delay_ms(650);
		
		jiazi(zhua);           //抓物料1
		
		Emm_V5_Pos_Control(5, 0, speed_hua, speed_hua_jia, 6200*4, 0, 0);       //升降台上升
		
//	delay_ms(500);
//		
//	zhou(zhua1);	        //轴转动，准备将物料1放进托盘
//		
//		Emm_V5_Pos_Control(5, 1, speed_hua, speed_hua_jia, 1800, 0, 0);    //升降台下降
//		
//	delay_ms(300);
//	
//		jiazi(fang);     //将物料1放进托盘
		
//		Emm_V5_Pos_Control(5, 0, speed_hua, speed_hua_jia, 1800, 0, 0);      //升降台上升
//	
//	delay_ms(300);
//		
//		zhou(guiwei);    //夹子正对物料
//		
}


void fnwuliao2(void)
{
	Emm_V5_Pos_Control(5, 1, speed_hua, speed_hua_jia, 6200*4, 0, 0);        //升降台下降
	
	delay_ms(650);
		
		jiazi(zhua);           //抓物料2
		
		Emm_V5_Pos_Control(5, 0, speed_hua, speed_hua_jia, 6200*4, 0, 0);       //升降台上升
		
//	delay_ms(500);
//		
//	zhou(zhua2);	        //轴转动，准备将物料2放进托盘
//		
//		Emm_V5_Pos_Control(5, 1, speed_hua, speed_hua_jia, 1800, 0, 0);    //升降台下降
//		
//	delay_ms(300);
//	
//		jiazi(fang);     //将物料2放进托盘
//		
//		Emm_V5_Pos_Control(5, 0, speed_hua, speed_hua_jia, 1800, 0, 0);      //升降台上升
//	
//	delay_ms(300);
//		
//		zhou(guiwei);    //夹子正对物料
		
}


void fnwuliao3(void)
{
	
	Emm_V5_Pos_Control(5, 1, speed_hua, speed_hua_jia, 6200*4, 0, 0);        //升降台下降
	
	delay_ms(650);
		
		jiazi(zhua);           //抓物料3
		
		Emm_V5_Pos_Control(5, 0, speed_hua, speed_hua_jia, 6200*4, 0, 0);       //升降台上升
		
	delay_ms(550);
		
	zhou(zhua3);	        //轴转动，准备将物料2放进托盘
		
		Emm_V5_Pos_Control(5, 1, speed_hua, speed_hua_jia, 1800*4, 0, 0);    //升降台下降
		
	delay_ms(650);
	
		jiazi(fang);       //将物料3放进托盘
		
		Emm_V5_Pos_Control(5, 0, speed_hua, speed_hua_jia, 1800*4, 0, 0);      //升降台上升
	
	delay_ms(250);
//		
//		zhou(guiwei);    //夹子正对物料
		
}


void maduo1(void)
{
	jiazi(fang);
	zhou(fang1);       //转至物料1位置
	
	Emm_V5_Pos_Control(5, 1, speed_hua, speed_hua_jia, 2000*4, 0, 0);    //升降台降
	
	delay_ms(650);

	jiazi(zhua);     //抓物料1
		
	Emm_V5_Pos_Control(5, 0, speed_hua, speed_hua_jia, 2000*4, 0, 0);      //升降台升
	
	delay_ms(150);
	
	zhou(guiwei);       //转回0位置
	
	Emm_V5_Pos_Control(5, 1, speed_hua, speed_hua_jia, 2600*4, 0, 0);     //升降台降
	
	delay_ms(650);
	
	jiazi(fang);	    //放物料1
		
	Emm_V5_Pos_Control(5, 0, speed_hua, speed_hua_jia, 2600*4, 0, 0);    //升降台升
	
	delay_ms(200);
	
}	


void maduo2(void)
{
	jiazi(fang);
	zhou(fang2);       //转至物料2位置
	
	Emm_V5_Pos_Control(5, 1, speed_hua, speed_hua_jia, 2000*4, 0, 0);    //升降台降
	
	delay_ms(650);

	jiazi(zhua);     //抓物料2
		
	Emm_V5_Pos_Control(5, 0, speed_hua, speed_hua_jia, 2000*4, 0, 0);      //升降台升
	
	delay_ms(150);
	
	zhou(guiwei);       //转回0位置
	
	Emm_V5_Pos_Control(5, 1, speed_hua_fang, speed_hua_jia_fang, 2600*4, 0, 0);     //升降台降
	
	delay_ms(650);
	
	jiazi(fang);	    //放物料1
		
	Emm_V5_Pos_Control(5, 0, speed_hua, speed_hua_jia, 2600*4, 0, 0);    //升降台升
	
	delay_ms(200);
	
}	

void maduo_z(int WL)
{
	delay_ms(10);
		Emm_V5_Pos_Control(1, 1, 50, 150, 1500, 0, 0);
		delay_ms(10);
		Emm_V5_Pos_Control(2, 1, 50, 150, 1500, 0, 0);
		delay_ms(10);
		Emm_V5_Pos_Control(3, 0, 50, 150, 1500, 0, 0);
		delay_ms(10);
		Emm_V5_Pos_Control(4, 0, 50, 150, 1500, 0, 0);
	delay_ms(300);
	jiazi(fang);
	zhou(WL);       //转至物料3位置
	
	Emm_V5_Pos_Control(5, 1, speed_hua, speed_hua_jia, 2000*4, 0, 0);    //升降台降
	
	delay_ms(650);

	jiazi(zhua);     //抓物料3
		
	Emm_V5_Pos_Control(5, 0, speed_hua, speed_hua_jia, 2000*4, 0, 0);      //升降台升
	
	delay_ms(150);
	
	zhou(guiwei);       //转回0位置
	
	delay_ms(10);
		Emm_V5_Pos_Control(1, 0, 50, 150, 1500, 0, 0);
		delay_ms(10);
		Emm_V5_Pos_Control(2, 0, 50, 150, 1500, 0, 0);
		delay_ms(10);
		Emm_V5_Pos_Control(3, 1, 50, 150, 1500, 0, 0);
		delay_ms(10);
		Emm_V5_Pos_Control(4, 1, 50, 150, 1500, 0, 0);
		delay_ms(300);
	
	Emm_V5_Pos_Control(5, 1, speed_hua_fang, speed_hua_jia_fang, 2600*4, 0, 0);     //升降台降
	
	delay_ms(650);
	
	
	
	jiazi(fang);	    //放物料1
		
	Emm_V5_Pos_Control(5, 0, speed_hua, speed_hua_jia, 2600*4, 0, 0);    //升降台升
	
//	delay_ms(350);

	
}	


void maduo3(void)
{
	jiazi(fang);
	zhou(fang3);       //转至物料3位置
	
	Emm_V5_Pos_Control(5, 1, speed_hua, speed_hua_jia, 2000*4, 0, 0);    //升降台降
	
	delay_ms(650);

	jiazi(zhua);     //抓物料3
		
	Emm_V5_Pos_Control(5, 0, speed_hua, speed_hua_jia, 2000*4, 0, 0);      //升降台升
	
	delay_ms(150);
	
	zhou(guiwei);       //转回0位置
	
	Emm_V5_Pos_Control(5, 1, speed_hua_fang, speed_hua_jia_fang, 2600*4, 0, 0);     //升降台降
	
	delay_ms(650);
	
	jiazi(fang);	    //放物料3
		
	Emm_V5_Pos_Control(5, 0, speed_hua, speed_hua_jia, 2600*4, 0, 0);    //升降台升
	
//	delay_ms(350);
	
}	


void dingweijiang(void)
{
//	StepMotor_Move(4500,0);
}

void dingweisheng(void)
{
//	StepMotor_Move(4500,1);
}


void zhuazikai(void)
{
	jiazi(120);
}

void zhuanpan0(void)
{
	zhou(guiwei);
}

void zhuanpanhui(void)
{
	zhou(fang1);
}

/* Standalone calibration test; existing task functions above are untouched. */
static uint8_t red_test_active = 0;
static const char *red_test_phase = "PREP";

int RedAlignTest_Display(void)
{
    char rows[4][17];
    uint8_t found, line, col;
    uint8_t wheel_state, wheel_done;
    char wheel_axis;
    const char *phase;
    int dx, dy;
    uint32_t mask;
    if (!red_test_active) return 0;
    mask = __get_PRIMASK();
    __disable_irq();
    found = maixcam_found;
    dx = maixcam_dx;
    dy = maixcam_dy;
    wheel_state = wl_wheel_wait_state;
    wheel_axis = wl_wheel_wait_axis;
    wheel_done = wheel_reply_done_mask;
    __set_PRIMASK(mask);
    snprintf(rows[0], sizeof(rows[0]), "RED %-5s F:%u   ",
             red_test_phase, (unsigned)found);
    if (found && dx >= -160 && dx <= 159 && dy >= -120 && dy <= 119)
    {
        snprintf(rows[1], sizeof(rows[1]), "X:%03d Y:%03d     ", 160 + dx, 120 + dy);
        snprintf(rows[2], sizeof(rows[2]), "DX:%+04d DY:%+04d ", dx, dy);
        snprintf(rows[3], sizeof(rows[3]), "EX:%+04d EY:%+04d ", -38 - dx, -71 - dy);
    }
    else
    {
        snprintf(rows[1], sizeof(rows[1]), "X:--- Y:---     ");
        snprintf(rows[2], sizeof(rows[2]), "DX:---- DY:---- ");
        snprintf(rows[3], sizeof(rows[3]), "EX:---- EY:---- ");
    }
    if (wheel_state != WL_WHEEL_IDLE)
    {
        phase = red_test_phase;
        if (wheel_state == WL_WHEEL_WAIT) phase = "DLY";
        snprintf(rows[0], sizeof(rows[0]), "%s %c M:%c%c%c%c", phase, wheel_axis,
                 (wheel_done & 1) ? '1' : '-', (wheel_done & 2) ? '2' : '-',
                 (wheel_done & 4) ? '3' : '-', (wheel_done & 8) ? '4' : '-');
    }
    for (line = 0; line < 4; ++line)
    {
        for (col = 0; rows[line][col] && col < 16; ++col)
            OLED_ShowChar(line + 1, col + 1, rows[line][col]);
        for (; col < 16; ++col)
            OLED_ShowChar(line + 1, col + 1, ' ');
    }
    return 1;
}

void RedAlignTest_Run(void)
{
    uint8_t addr;
    red_test_active = 1;
    red_test_phase = "PREP";
    delay_ms(1000);
    FSUS_ServoAngleReset(&usart2, 0);
    delay_ms(100);
    zhuazikai();
    zhuanpan0();
    delay_ms(500);

    red_test_phase = "ALIGN";
    WL_dingwei('1'); /* Existing one-pass position adjustment, red mode 0x01. */
    /* Stop wheel commands before lowering; no route or grasp follows. */
    for (addr = 1; addr <= 4; ++addr)
    {
        Emm_V5_Stop_Now(addr, 0);
        delay_ms(10);
    }
    delay_ms(500);
    Emm_V5_Pos_Control(5, 1, RED_ALIGN_TEST_SPEED_RPM,RED_ALIGN_TEST_ACCEL, RED_ALIGN_TEST_DOWN_PULSES, 0, 0);
    delay_ms(650);
    //red_test_phase = "HOLD"; /* Indicates program hold, not motor arrival feedback. */
	
	jiazi(zhua);
	delay_ms(500);
	Emm_V5_Pos_Control(5, 0, RED_ALIGN_TEST_SPEED_RPM,RED_ALIGN_TEST_ACCEL, RED_ALIGN_TEST_DOWN_PULSES, 0, 0);
	
	
    for (;;)
        delay_ms(100); /* Keep UART reception and OLED updates running forever. */
}
