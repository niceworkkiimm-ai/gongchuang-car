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


//轴ID：0（紫色）     归为：-8；   1：-154；   2（从左）：-187；  2（从右）：174    3：143；  
//爪子ID：1（绿色）   抓物料：0；  开爪子：-30；
//参数：串口，舵机ID，角度，速度，加速时间，减速时间，舵机执行功率（0），1
//步进电机 向上运动：0    向下运动：1
extern __IO uint8_t rxCmd[FIFO_SIZE];


int zhua1=-158;    //-4
int zhua2=173;     //-4
int zhua3=139;
int fang1=-158;
int fang3=139;
int fang2=-191;

int zhua=12;
int fang=-17;
int guiwei=-12;

int speed_hua=500;
int speed_hua_jia=500;

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


void wuliao1(void)
{
	//准备抓物料之前，爪子升到最高处并且张开60°，朝向物料
	
//	zhou(guiwei);            //夹子正对物料
	
	Emm_V5_Pos_Control(5, 1, speed_hua, speed_hua_jia, 1850*4, 0, 0);        //升降台下降
	//转盘高度降低增加300
	delay_ms(150);
		
		jiazi(zhua);           //抓物料
	
		delay_ms(150);
		
		Emm_V5_Pos_Control(5, 0, speed_hua, speed_hua_jia, 1600*4, 0, 0);       //升降台上升
		
	delay_ms(200);
		
	zhou(zhua1);	        //轴转动，准备放物料1
		
		Emm_V5_Pos_Control(5, 1, speed_hua, speed_hua_jia, 1800*4, 0, 0);    //升降台下降
		
	delay_ms(200);
	
		jiazi(fang);     //放物料
		
		Emm_V5_Pos_Control(5, 0, speed_hua, speed_hua_jia, 1800*4, 0, 0);      //升降台上升
	
	delay_ms(200);
		
		zhou(guiwei);    //夹子正对物料
		
}


void wuliao2(void)
{
	//准备抓物料之前，爪子升到最高处并且张开60°，朝向物料
	
//	zhou(guiwei);            //夹子正对物料
	
	Emm_V5_Pos_Control(5, 1, speed_hua, speed_hua_jia, 1600*4, 0, 0);        //升降台下降
	
	delay_ms(150);
		
		jiazi(zhua);           //抓物料
	
		delay_ms(150);
		
		Emm_V5_Pos_Control(5, 0, speed_hua, speed_hua_jia, 1600*4, 0, 0);       //升降台上升
		
	delay_ms(200);
		
	zhou(zhua2);	        //轴转动，准备放物料2
	delay_ms(300);
		
		Emm_V5_Pos_Control(5, 1, speed_hua, speed_hua_jia, 1800*4, 0, 0);    //升降台下降
		
	delay_ms(200);
	
		jiazi(fang);     //放物料
		
		Emm_V5_Pos_Control(5, 0, speed_hua, speed_hua_jia, 1800*4, 0, 0);      //升降台上升
	
	delay_ms(200);
		
		zhou(guiwei);    //夹子正对物料

}


void wuliao3(void)
{
	//准备抓物料之前，爪子升到最高处并且张开60°，朝向物料
	
//	zhou(guiwei);            //夹子正对物料
	
	Emm_V5_Pos_Control(5, 1, speed_hua, speed_hua_jia, 1600*4, 0, 0);        //升降台下降
	
	delay_ms(150);
		
		jiazi(zhua);           //抓物料
	
		delay_ms(150);
		
		Emm_V5_Pos_Control(5, 0, speed_hua, speed_hua_jia, 1600*4, 0, 0);       //升降台上升
		
	delay_ms(250);
		
	zhou(zhua3);	        //轴转动，准备放物料3
		
		Emm_V5_Pos_Control(5, 1, speed_hua, speed_hua_jia, 1800*4, 0, 0);    //升降台下降
		
	delay_ms(200);
	
		jiazi(fang);     //放物料
		
		Emm_V5_Pos_Control(5, 0, speed_hua, speed_hua_jia, 1800*4, 0, 0);      //升降台上升
	
	delay_ms(200);
		
//		zhou(guiwei);    //夹子正对物料

}


void fwuliao1(void)
{
	
	jiazi(fang);
	Emm_V5_Pos_Control(5, 0, speed_hua, speed_hua_jia, 6200*4, 0, 0);    //升降台升
	delay_ms(450);
	zhou(fang1);             //转至物料1位置
	
	Emm_V5_Pos_Control(5, 1, speed_hua, speed_hua_jia, 2000*4, 0, 0);    //升降台降
	
	delay_ms(250);
		
		jiazi(zhua);      //抓物料1
		
	Emm_V5_Pos_Control(5, 0, speed_hua, speed_hua_jia, 2000*4, 0, 0);    //升降台升
	
	delay_ms(150);
	
		zhou(guiwei);      //转回0位置
		
	Emm_V5_Pos_Control(5, 1, speed_hua_fang, speed_hua_jia_fang, 6400*4, 0, 0);    //升降台降
	
	delay_ms(600);
		
	jiazi(-60);      //放物料1

//	Emm_V5_Pos_Control(5, 0, speed_hua, speed_hua_jia, 6200, 0, 0);    //升降台升
	
//	delay_ms(250);
	
}	


void fwuliao2(void)
{
	jiazi(fang);
	Emm_V5_Pos_Control(5, 0, speed_hua, speed_hua_jia, 6400*4, 0, 0);    //升降台升
	delay_ms(450);
	zhou(fang2);              //转至物料2位置
	
	Emm_V5_Pos_Control(5, 1, speed_hua, speed_hua_jia, 2000*4, 0, 0);        //升降台降
	
	delay_ms(250);
		 
		jiazi(zhua);         //抓物料2
		
	Emm_V5_Pos_Control(5, 0, speed_hua, speed_hua_jia, 2000*4, 0, 0);            //升降台升
	
	delay_ms(150);
	
		zhou(guiwei);       //转回0位置
		
	Emm_V5_Pos_Control(5, 1, speed_hua_fang, speed_hua_jia_fang, 6400*4, 0, 0);           //升降台降
	
	delay_ms(600);
		
	jiazi(-60);        //放物料2

//	Emm_V5_Pos_Control(5, 0, speed_hua, speed_hua_jia, 6200, 0, 0);     //升降台升
	
//	delay_ms(250);
	
}	


void fwuliao3(void)
{
	jiazi(fang);
	Emm_V5_Pos_Control(5, 0, speed_hua, speed_hua_jia, 6400*4, 0, 0);    //升降台升
	delay_ms(450);
	zhou(fang3);           //转至物料3位置
	
	Emm_V5_Pos_Control(5, 1, speed_hua, speed_hua_jia, 2000*4, 0, 0);        //升降台降
	
	delay_ms(250);
		
		jiazi(zhua);                //抓物料2
		
	Emm_V5_Pos_Control(5, 0, speed_hua, speed_hua_jia, 2000*4, 0, 0);             //升降台升
	
	delay_ms(150);
	
		zhou(guiwei);           //转回0位置
		
	Emm_V5_Pos_Control(5, 1, speed_hua_fang, speed_hua_jia_fang, 6400*4, 0, 0);         //升降台降
	
	delay_ms(600);
		
	jiazi(fang);        //放物料3

	Emm_V5_Pos_Control(5, 0, speed_hua, speed_hua_jia, 6400*4, 0, 0);        //升降台升
	
//	delay_ms(250);
	
}	


void fnwuliao1(void)
{
	
	Emm_V5_Pos_Control(5, 1, speed_hua, speed_hua_jia, 6200*4, 0, 0);        //升降台下降
	
	delay_ms(550);
		
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
	
	delay_ms(550);
		
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
	
	delay_ms(550);
		
		jiazi(zhua);           //抓物料3
		
		Emm_V5_Pos_Control(5, 0, speed_hua, speed_hua_jia, 6200*4, 0, 0);       //升降台上升
		
	delay_ms(550);
		
	zhou(zhua3);	        //轴转动，准备将物料2放进托盘
		
		Emm_V5_Pos_Control(5, 1, speed_hua, speed_hua_jia, 1800*4, 0, 0);    //升降台下降
		
	delay_ms(250);
	
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
	
	delay_ms(250);

	jiazi(zhua);     //抓物料1
		
	Emm_V5_Pos_Control(5, 0, speed_hua, speed_hua_jia, 2000*4, 0, 0);      //升降台升
	
	delay_ms(150);
	
	zhou(guiwei);       //转回0位置
	
	Emm_V5_Pos_Control(5, 1, speed_hua, speed_hua_jia, 2600*4, 0, 0);     //升降台降
	
	delay_ms(250);
	
	jiazi(fang);	    //放物料1
	delay_ms(200);	
	Emm_V5_Pos_Control(5, 0, speed_hua, speed_hua_jia, 2600*4, 0, 0);    //升降台升
	
	delay_ms(200);
	
}	


void maduo2(void)
{
	jiazi(fang);
	zhou(fang2);       //转至物料2位置
	
	Emm_V5_Pos_Control(5, 1, speed_hua, speed_hua_jia, 2000*4, 0, 0);    //升降台降
	
	delay_ms(250);

	jiazi(zhua);     //抓物料2
		
	Emm_V5_Pos_Control(5, 0, speed_hua, speed_hua_jia, 2000*4, 0, 0);      //升降台升
	
	delay_ms(150);
	
	zhou(guiwei);       //转回0位置
	
	Emm_V5_Pos_Control(5, 1, speed_hua_fang, speed_hua_jia_fang, 2600*4, 0, 0);     //升降台降
	
	delay_ms(300);
	
	jiazi(fang);	    //放物料1
	delay_ms(200);		
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
	
	delay_ms(250);

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
	
	delay_ms(300);
	
	
	
	jiazi(fang);	    //放物料1
	delay_ms(200);		
	Emm_V5_Pos_Control(5, 0, speed_hua, speed_hua_jia, 2600*4, 0, 0);    //升降台升
	
//	delay_ms(350);

	
}	


void maduo3(void)
{
	jiazi(fang);
	zhou(fang3);       //转至物料3位置
	
	Emm_V5_Pos_Control(5, 1, speed_hua, speed_hua_jia, 2000*4, 0, 0);    //升降台降
	
	delay_ms(250);

	jiazi(zhua);     //抓物料3
		
	Emm_V5_Pos_Control(5, 0, speed_hua, speed_hua_jia, 2000*4, 0, 0);      //升降台升
	
	delay_ms(150);
	
	zhou(guiwei);       //转回0位置
	
	Emm_V5_Pos_Control(5, 1, speed_hua_fang, speed_hua_jia_fang, 2600*4, 0, 0);     //升降台降
	
	delay_ms(300);
	
	jiazi(fang);	    //放物料3
	delay_ms(200);		
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
	jiazi(-60);
}

void zhuanpan0(void)
{
	zhou(-12);
}

void zhuanpanhui(void)
{
	zhou(-154);
}
