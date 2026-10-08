/* MaixCAM 接口已对齐。以下注释中的旧抓放流程仍未启用；启用时必须检查 VisionResult。 */
#include "delay.h"
#include "Emm_V5.h"
#include "usart.h"
#include "car.h"
#include "openmv.h"
#include "wuliao.h"
#include "uart_5.h"
#include "uart_4.h"
#include "uart_3.h"
int speed_hua2=500;
int speed_hua_jia2=500;
int speed=500;
int qi=1;
extern float anglea;
extern volatile float angle_offset;
extern volatile float global_angle;

void zancun_na(dir,speed,location,WL){
		if (dir == 1) 
	{
		rxCmd[1] = 0x00;
		rxCmd[2] = 0x00;
		rxCmd[3] = 0x00;
		delay_ms(10);
		Emm_V5_Pos_Control(1, 0, speed, 150, location, 0, 0);
		delay_ms(10);
		Emm_V5_Pos_Control(2, 1, speed, 150, location, 0, 0);
		delay_ms(10);
		Emm_V5_Pos_Control(3, 0, speed, 150, location, 0, 0);
		delay_ms(10);
		Emm_V5_Pos_Control(4, 1, speed, 150, location, 0, 0);
		
		delay_ms(350);
		if(WL == 1)
			zhou(zhua1);	        //轴转动，准备将物料1放进托盘
		else if (WL == 2)
			zhou(zhua2);
		
		delay_ms(200);
		
		Emm_V5_Pos_Control(5, 1, speed_hua2, speed_hua_jia2, 1800*4, 0, 0);    //升降台下降
		
	delay_ms(250);
	
		jiazi(fang);     //将物料2放进托盘
		
		Emm_V5_Pos_Control(5, 0, speed_hua2, speed_hua_jia2, 1800*4, 0, 0);      //升降台上升
	
	delay_ms(250);
		
		zhou(guiwei);    //夹子正对物料

//		while(1)
//		{
//			if (rxCmd[1] == 0xFD && rxCmd[2] == 0x9F && rxCmd [3] == 0x6B)
//			{	
//				delay_ms(20);
//				break;
//		}
//		}
	}
	
	if (dir == 2)
	{
		rxCmd[1] = 0x00;
		rxCmd[2] = 0x00;
		rxCmd[3] = 0x00;
		delay_ms(10);
		Emm_V5_Pos_Control(1, 1, speed, 150, location, 0, 0);
		delay_ms(10);
		Emm_V5_Pos_Control(2, 0, speed, 150, location, 0, 0);
		delay_ms(10);
		Emm_V5_Pos_Control(3, 1, speed, 150, location, 0, 0);
		delay_ms(10);
		Emm_V5_Pos_Control(4, 0, speed, 150, location, 0, 0);
		
		
		delay_ms(350);
		if(WL == 1)
			zhou(zhua1);	        //轴转动，准备将物料1放进托盘
		else if (WL == 2)
			zhou(zhua2);
		
		Emm_V5_Pos_Control(5, 1, speed_hua2, speed_hua_jia2, 1800*4, 0, 0);    //升降台下降
		
	delay_ms(250);
	
		jiazi(fang);     //将物料2放进托盘
		
		Emm_V5_Pos_Control(5, 0, speed_hua2, speed_hua_jia2, 1800*4, 0, 0);      //升降台上升
	
	delay_ms(250);
		
		zhou(guiwei);    //夹子正对物料
		

//		while(1)
//		{
//			if (rxCmd[1] == 0xFD && rxCmd[2] == 0x9F && rxCmd [3] == 0x6B)
//			{	
//				delay_ms(20);
//				break;
//		}
//		}
		
	}
}


void WL_3(int location){
		rxCmd[1] = 0x00;
		rxCmd[2] = 0x00;
		rxCmd[3] = 0x00;
		delay_ms(10);
		Emm_V5_Pos_Control(1, 1, speed, 150, location, 0, 0);
		delay_ms(10);
		Emm_V5_Pos_Control(2, 0, speed, 150, location, 0, 0);
		delay_ms(10);
		Emm_V5_Pos_Control(3, 1, speed, 150, location, 0, 0);
		delay_ms(10);
		Emm_V5_Pos_Control(4, 0, speed, 150, location, 0, 0);
	
	delay_ms(400);
	zhou(zhua3);	        //轴转动，准备放物料3
		
		Emm_V5_Pos_Control(5, 1, speed_hua2, speed_hua_jia2, 1800*4, 0, 0);    //升降台下降
		
	delay_ms(200);
	
		jiazi(fang);     //放物料
		
		Emm_V5_Pos_Control(5, 0, speed_hua2, speed_hua_jia2, 1800*4, 0, 0);      //升降台上升

		while(1)
		{
			if (rxCmd[1] == 0xFD && rxCmd[2] == 0x9F && rxCmd [3] == 0x6B)
			{	
				delay_ms(20);
				break;
		}
		}
}

void fuwei(void)
{
    MaixCAM_ResetTarget();
}



    


void yuanliao()
{
		car(9,speed,4000*4);//
			delay_ms(50);
	car(0,0,0);
	delay_ms(50);
	car(2,speed,11000*4);//前进
	car(0,0,0);
	delay_ms(50);
//	if (shunxuma == 0)   // 检测到扫码未完成
//    {
//					car(5,speed,8900*4);//原地转半圈
//					angle_offset = global_angle;  // 强制清零：当前角度变为新(0,0,0)
//			    qi++;
//    }
			car(6,speed,4450*4);//转90度
			car(0,-90,0);
	delay_ms(50);
			car(2,speed,12000*4);//直走
			//car(13,speed,4500*4);
	delay_ms(50);
		car(6,speed,4450*4);//转90度
			car(0,179,0);
	delay_ms(50);
			car(2,speed,11500*4);
	delay_ms(50);			
			car(6,speed,4450*4);//转90度		
	
	
//	zhuazikai();
//	zhuanpan0();
//	delay_ms(300);
//	fuwei();
//	
//	WL_dingwei(MaixCAM_ColorFromAscii(saoma_data[0]));
//	delay_ms(50);
//	wuliao1();
//	fuwei();
//	
//	zhuazikai();

//	if (saoma_data[1] == '1'){
//		MaixCAM_SetMode(MAIXCAM_MODE_RED);
//		while(1){
//			fuwei();
//		if (MaixCAM_TargetFound()){
//			delay_ms(50);
//			
//			wuliao2();
//			break;
//		}
//	}
//	}
//	else if (saoma_data[1] == '2'){
//		MaixCAM_SetMode(MAIXCAM_MODE_YELLOW);
//		while(1){
//			fuwei();
//		if (MaixCAM_TargetFound()){
//			delay_ms(50);
//			
//			wuliao2();
//			break;
//		}
//	}
//	}
//	else if (saoma_data[1] == '3'){
//		MaixCAM_SetMode(MAIXCAM_MODE_BLUE);
//		while(1){
//						fuwei();
//		if (MaixCAM_TargetFound()){
//			delay_ms(50);

//			wuliao2();	
//			break;
//		}
//	}
//	}
//	
//	zhuazikai();
//	
//	if (saoma_data[2] == '1'){
//		MaixCAM_SetMode(MAIXCAM_MODE_RED);
//		while(1){
//			fuwei();
//		if (MaixCAM_TargetFound()){
//			delay_ms(50);
//			
//			wuliao3();
//			break;
//		}
//	}
//	}
//	else if (saoma_data[2] == '2'){
//		MaixCAM_SetMode(MAIXCAM_MODE_YELLOW);
//		while(1){
//			fuwei();
//		if (MaixCAM_TargetFound()){
//			delay_ms(50);
//			
//			wuliao3();
//			break;
//		}
//	}
//	}
//	else if (saoma_data[2] == '3'){
//		MaixCAM_SetMode(MAIXCAM_MODE_BLUE);
//		while(1){
//			fuwei();
//		if (MaixCAM_TargetFound()){
//			delay_ms(50);
//			
//			wuliao3();
//			break;
//		}
//	}
//	}

}


void cujiagong()
{
	
//		
//	WL_3(5500*4);
		
//	car(2,700,5000*4);
//	car(6,speed,4450*4);//转90度		
	//car(12,speed,4250*4);
	car(6,speed,4450*4);
	car(0,0,0);
	delay_ms(50);
	car(2,speed,23000*4);
	delay_ms(50);
//	zhou(guiwei);    //夹子正对物料
//	Emm_V5_Pos_Control(5, 1, speed_hua2, speed_hua_jia2, 6200*4, 0, 0);    //升降台降
	car(6,speed,4450*4);

//car(0,-90,0);
//	delay_ms(50);
//	if (anglea<179 && anglea>=0)
//		car(0,179,0);
//	else if( anglea>-179 && anglea <=0)
//		car(0,-179,0);
//	
//	delay_ms(50);

//	//***********放物料*************
//	if (saoma_data[0] == '1'){
//		car(22,500,1850*4);
//			fuwei();
//		vision(MAIXCAM_MODE_RING);
//		fwuliao1();
//		
//		if (saoma_data[1] == '2'){
//			car(21,500,1850*4);
//				fuwei();
//			vision(MAIXCAM_MODE_RING);
//			fwuliao2();
//			
//			if (saoma_data[2]=='3'){
//				car(21,500,1850*4);
//				fuwei();
//				vision(MAIXCAM_MODE_RING);
//				fwuliao3();
//				
//				if (anglea<179 && anglea>=0)
//					car(0,179,0);
//				else if( anglea>-179 && anglea <=0)
//					car(0,-179,0);
//				delay_ms(50);
//			//**************抓物料**************	
//				car(2,500,3700*4);
//				fnwuliao1();
//				zancun_na(1,500,1850*4,1);
//				fnwuliao2();
//				zancun_na(1,500,1850*4,2);
//				fnwuliao3();
//			}
//			}
//		
//		if (saoma_data[1] == '3'){
//			car(21,500,3700*4);
//				fuwei();
//			vision(MAIXCAM_MODE_RING);
//			fwuliao2();
//			if (saoma_data[2] == '2'){
//				car(22,500,1850*4);
//				fuwei();
//				vision(MAIXCAM_MODE_RING);
//				fwuliao3();
//				
//				if (anglea<179 && anglea>=0)
//					car(0,179,0);
//				else if( anglea>-179 && anglea <=0)
//					car(0,-179,0);
//				delay_ms(50);
//				//**************抓物料**************	
//				car(2,500,1850*4);
//				fnwuliao1();
//				zancun_na(1,500,3700*4,1);
//				fnwuliao2();
//				zancun_na(2,500,1850*4,2);
//				fnwuliao3();
//			}
//		}
//	}
//	
//	
//	if (saoma_data[0] == '2'){
//			fuwei();
//		vision(MAIXCAM_MODE_RING);
//		fwuliao1();
//		
//		if (saoma_data[1] == '1'){
//			car(22,500,1850*4);
//				fuwei();
//			vision(MAIXCAM_MODE_RING);
//			fwuliao2();
//			if (saoma_data[2] == '3'){
//				
//				car(21,500,3700*4);
//				fuwei();
//				vision(MAIXCAM_MODE_RING);
//				fwuliao3();
//				
//				if (anglea<179 && anglea>=0)
//					car(0,179,0);
//				else if( anglea>-179 && anglea <=0)
//					car(0,-179,0);
//				delay_ms(50);
//				//**************抓物料**************	
//				car(2,500,1850*4);
//				fnwuliao1();
//				zancun_na(2,500,1850*4,1);
//				fnwuliao2();
//				zancun_na(1,500,3700*4,2);
//				fnwuliao3();
//			}
//		}
//		
//		if (saoma_data[1] == '3'){
//			car(21,500,1850*4);
//				fuwei();
//			vision(MAIXCAM_MODE_RING);
//			fwuliao2();
//			if (saoma_data[2] == '1'){
//				car(22,500,3700*4);
//				fuwei();
//				vision(MAIXCAM_MODE_RING);
//				fwuliao3();
//				
//				if (anglea<179 && anglea>=0)
//					car(0,179,0);
//				else if( anglea>-179 && anglea <=0)
//					car(0,-179,0);
//				delay_ms(50);
//				//**************抓物料**************	
//				car(1,500,1850*4);
//				fnwuliao1();
//				zancun_na(1,500,1850*4,1);
//				fnwuliao2();
//				zancun_na(2,500,3900*4,2);
//				fnwuliao3();
//			}
//		}
//	}
//	
//	
//	if (saoma_data[0] == '3'){
//		car(21,500,1850*4);
//		fuwei();
//		vision(MAIXCAM_MODE_RING);
//		fwuliao1();
//		
//		if (saoma_data[1] == '2'){
//			car(22,500,1850*4);
//				fuwei();
//			vision(MAIXCAM_MODE_RING);
//			fwuliao2();
//			if (saoma_data[2] == '1'){
//				car(22,500,1850*4);
//				fuwei();
//				vision(MAIXCAM_MODE_RING);
//				fwuliao3();
//				
//				if (anglea<179 && anglea>=0)
//					car(0,179,0);
//				else if( anglea>-179 && anglea <=0)
//					car(0,-179,0);
//				delay_ms(50);
//				//**************抓物料**************	
//				car(1,500,3750*4);
//				fnwuliao1();
//				zancun_na(2,500,1850*4,1);
//				fnwuliao2();
//				zancun_na(2,500,1850*4,2);
//				fnwuliao3();
//			}
//		}
//		
//		if (saoma_data[1] == '1'){
//			car(22,500,3700*4);
//				fuwei();
//			vision(MAIXCAM_MODE_RING);
//			fwuliao2();
//			if (saoma_data[2] == '2'){
//				car(21,500,1850*4);
//				fuwei();
//				vision(MAIXCAM_MODE_RING);
//				fwuliao3();
//				
//				if (anglea<179 && anglea>=0)
//					car(0,179,0);
//				else if( anglea>-179 && anglea <=0)
//					car(0,-179,0);
//				delay_ms(50);
//				//**************抓物料**************	
//				car(1,500,1850*4);
//				fnwuliao1();
//				zancun_na(2,500,3700*4,1);
//				fnwuliao2();
//				zancun_na(1,500,1850*4,2);
//				fnwuliao3();
//			}
//		}
//	}
//	
//	
//	
//	
}

void zancunqu()
{
//	if (anglea<179 && anglea>=0)
//		car(0,179,0);
//	else if( anglea>-179 && anglea <=0)
//		car(0,-179,0);
//	delay_ms(50);
//	if (saoma_data[2] == '1'){
	//	car(11,700,5870*4);
	
////		WL_3(8800*4);
//	}
//	else if (saoma_data[2] == '2'){
//		car(11,700,7725*4);
////		WL_3(10650*4);
//	}
//	else if (saoma_data[2] == '3'){
//		car(11,700,9575*4);
////		WL_3(12500*4);
//	}
		
//	car(2,700,11000);
//	car(6,speed,4450*4);
	car(0,-90,0);
	delay_ms(50);
	car(2,speed,11000*4);
	delay_ms(50);
		car(6,speed,4450*4);
		if (anglea<179 && anglea>=0)
					car(0,179,0);
				else if( anglea>-179 && anglea <=0)
					car(0,-179,0);
				delay_ms(50);
					car(2,speed,11000*4);
				delay_ms(50);
//	zhou(guiwei);    //夹子正对物料
//	delay_ms(100);
//	Emm_V5_Pos_Control(5, 1, speed_hua2, speed_hua_jia2, 6200*4, 0, 0);    //升降台降

//	
//	car(0,90,0);
//	delay_ms(50);
//	
//		//***********放物料*************
//	if (saoma_data[0] == '1'){
//		car(2,500,1850*4);
//			fuwei();
//		vision(MAIXCAM_MODE_RING);
//		fwuliao1();
//		
//		if (saoma_data[1] == '2'){
//			car(1,500,1850*4);
//				fuwei();
//			vision(MAIXCAM_MODE_RING);
//			fwuliao2();
//			
//			if (saoma_data[2]=='3'){
//				car(1,500,1850*4);
//				fuwei();
//				vision(MAIXCAM_MODE_RING);
//				fwuliao3();

//			}
//			}
//		
//		if (saoma_data[1] == '3'){
//			car(1,500,3700*4);
//				fuwei();
//			vision(MAIXCAM_MODE_RING);
//			fwuliao2();
//			if (saoma_data[2] == '2'){
//				car(2,500,1850*4);
//				fuwei();
//				vision(MAIXCAM_MODE_RING);
//				fwuliao3();
//				
//			}
//		}
//	}
//	
//	
//	if (saoma_data[0] == '2'){
//	fuwei();
//		vision(MAIXCAM_MODE_RING);
//		fwuliao1();
//		
//		if (saoma_data[1] == '1'){
//			car(2,500,1850*4);
//				fuwei();
//			vision(MAIXCAM_MODE_RING);
//			fwuliao2();
//			if (saoma_data[2] == '3'){
//				car(1,500,3700*4);
//				fuwei();
//				vision(MAIXCAM_MODE_RING);
//				fwuliao3();
//				
//			}
//		}
//		
//		if (saoma_data[1] == '3'){
//			car(1,500,1850*4);
//				fuwei();
//			vision(MAIXCAM_MODE_RING);
//			fwuliao2();
//			if (saoma_data[2] == '1'){
//				car(2,500,3700*4);
//				fuwei();
//				vision(MAIXCAM_MODE_RING);
//				fwuliao3();
//				
//			}
//		}
//	}
//	
//	
//	if (saoma_data[0] == '3'){
//		car(1,500,1850*4);
//			fuwei();
//		vision(MAIXCAM_MODE_RING);
//		fwuliao1();
//		
//		if (saoma_data[1] == '2'){
//			car(2,500,1850*4);
//				fuwei();
//			vision(MAIXCAM_MODE_RING);
//			fwuliao2();
//			if (saoma_data[2] == '1'){
//				car(2,500,1850*4);
//				fuwei();
//				vision(MAIXCAM_MODE_RING);
//				fwuliao3();

//			}
//		}
//		
//		if (saoma_data[1] == '1'){
//			car(2,500,3700*4);
//				fuwei();
//			vision(MAIXCAM_MODE_RING);
//			fwuliao2();
//			if (saoma_data[2] == '2'){
//				car(1,500,1850*4);
//				fuwei();
//				vision(MAIXCAM_MODE_RING);
//				fwuliao3();
//				
//			}
//		}
//	}
//	
}


//**************************************第二圈****************************************

void yuanliao2()
{
//	if (saoma_data[2] == '1'){
//		delay_ms(350);
//		zhou(fang1);
//		car(11,speed,6075*4);
//	}
//	else if (saoma_data[2] == '2'){
//		delay_ms(350);
//		zhou(fang1);
		//car(11,speed,7925*4);
//	}
//	else if (saoma_data[2] == '3'){
//		delay_ms(350);
//		zhou(fang1);
//		car(11,speed,9775*4);
//	}
////	car(6,speed,4450*4);
	
	
		if (anglea<179 && anglea>=0)
					car(0,179,0);
				else if( anglea>-179 && anglea <=0)
					car(0,-179,0);
				delay_ms(50);
    car(2,speed,11500*4);
				if (anglea<179 && anglea>=0)
					car(0,179,0);
				else if( anglea>-179 && anglea <=0)
					car(0,-179,0);
				delay_ms(50);
		car(6,speed,4450*4);
	car(0,90,0);
	delay_ms(50);
				
	car(2,speed,10000*4);
				delay_ms(50);
//	zhuazikai();
//	zhuanpan0();
//	delay_ms(300);
//	
//	fuwei();
//	WL_dingwei(MaixCAM_ColorFromAscii(saoma_data[4]));
//	delay_ms(50);
//	wuliao1();
//	
//	zhuazikai();
//	
//	if (saoma_data[5] == '1'){
//		MaixCAM_SetMode(MAIXCAM_MODE_RED);
//		while(1){
//			fuwei();
//		if (MaixCAM_TargetFound()){
//			delay_ms(50);
//			fuwei();
//			wuliao2();
//			break;
//		}
//	}
//	}
//	else if (saoma_data[5] == '2'){
//		MaixCAM_SetMode(MAIXCAM_MODE_YELLOW);
//		while(1){
//			fuwei();
//		if (MaixCAM_TargetFound()){
//			delay_ms(50);
//			
//			wuliao2();
//			break;
//		}
//	}
//	}
//	else if (saoma_data[5] == '3'){
//		MaixCAM_SetMode(MAIXCAM_MODE_BLUE);
//		while(1){
//		fuwei();
//		if (MaixCAM_TargetFound()){
//			delay_ms(50);

//			wuliao2();	
//			break;
//		}
//	}
//	}
//	
//	zhuazikai();
//	
//	if (saoma_data[6] == '1'){
//		MaixCAM_SetMode(MAIXCAM_MODE_RED);
//		while(1){
//			fuwei();
//		if (MaixCAM_TargetFound()){
//			delay_ms(50);
//			
//			wuliao3();
//			break;
//		}
//	}
//	}
//	else if (saoma_data[6] == '2'){
//		MaixCAM_SetMode(MAIXCAM_MODE_YELLOW);
//		while(1){
//			fuwei();
//		if (MaixCAM_TargetFound()){
//			delay_ms(50);
//			
//			wuliao3();
//			break;
//		}
//	}
//	}
//	else if (saoma_data[6] == '3'){
//		MaixCAM_SetMode(MAIXCAM_MODE_BLUE);
//		while(1){
//			fuwei();
//		if (MaixCAM_TargetFound()){
//			delay_ms(50);
//			
//			wuliao3();
//			break;
//		}
//	}
//	}
//	
}


void cujiagong2()
{
	
//	WL_3(5500*4);
	
//	car(2,700,5000);
//	car(10,speed,4050*4);
//	car(0,-90,0);
//	delay_ms(50);
//	car(2,speed,19000*4);
	car(6,speed,4450*4);
	delay_ms(50);
	car(2,speed,22500*4);
	
//	zhou(guiwei);    //夹子正对物料
//	Emm_V5_Pos_Control(5, 1, speed_hua2, speed_hua_jia2, 6200*4, 0, 0);    //升降台降
	
	delay_ms(50);
	car(6,speed,4450*4);
//	car(0,179,0);
//	delay_ms(50);
//	
//	//***********放物料*************
//	if (saoma_data[4] == '1'){
//		car(22,500,1850*4);
//			fuwei();
//		vision(MAIXCAM_MODE_RING);
//		fwuliao1();
//		
//		if (saoma_data[5] == '2'){
//			car(21,500,1850*4);
//				fuwei();
//			vision(MAIXCAM_MODE_RING);
//			fwuliao2();
//			
//			if (saoma_data[6]=='3'){
//				car(21,500,1850*4);
//				fuwei();
//				vision(MAIXCAM_MODE_RING);
//				fwuliao3();
//				
//				if (anglea<179 && anglea>=0)
//					car(0,179,0);
//				else if( anglea>-179 && anglea <=0)
//					car(0,-179,0);
//				delay_ms(50);
//			//**************抓物料**************	
//				car(2,500,3700*4);
//				fnwuliao1();
//				zancun_na(1,500,1850*4,1);
//				fnwuliao2();
//				zancun_na(1,500,1850*4,2);
//				fnwuliao3();
//			}
//			}
//		
//		if (saoma_data[5] == '3'){
//			car(21,500,3700*4);
//				fuwei();
//			vision(MAIXCAM_MODE_RING);
//			fwuliao2();
//			if (saoma_data[6] == '2'){
//				car(22,500,1850*4);
//				fuwei();
//				vision(MAIXCAM_MODE_RING);
//				fwuliao3();
//				
//				if (anglea<179 && anglea>=0)
//					car(0,179,0);
//				else if( anglea>-179 && anglea <=0)
//					car(0,-179,0);
//				delay_ms(50);
//				//**************抓物料**************	
//				car(2,500,1850*4);
//				fnwuliao1();
//				zancun_na(1,500,3700*4,1);
//				fnwuliao2();
//				zancun_na(2,500,1850*4,2);
//				fnwuliao3();
//			}
//		}
//	}
//	
//	
//	if (saoma_data[4] == '2'){
//	fuwei();
//		vision(MAIXCAM_MODE_RING);
//		fwuliao1();
//		
//		if (saoma_data[5] == '1'){
//			car(22,500,1850*4);
//				fuwei();
//			vision(MAIXCAM_MODE_RING);
//			fwuliao2();
//			if (saoma_data[6] == '3'){
//				car(21,500,3700*4);
//				fuwei();
//				vision(MAIXCAM_MODE_RING);
//				fwuliao3();
//				
//				if (anglea<179 && anglea>=0)
//					car(0,179,0);
//				else if( anglea>-179 && anglea <=0)
//					car(0,-179,0);
//				delay_ms(50);
//				//**************抓物料**************	
//				car(2,500,1850*4);
//				fnwuliao1();
//				zancun_na(2,500,1850*4,1);
//				fnwuliao2();
//				zancun_na(1,500,3700*4,2);
//				fnwuliao3();
//			}
//		}
//		
//		if (saoma_data[5] == '3'){
//			car(21,500,1850*4);
//				fuwei();
//			vision(MAIXCAM_MODE_RING);
//			fwuliao2();
//			if (saoma_data[6] == '1'){
//				car(22,500,3700*4);
//				fuwei();
//				vision(MAIXCAM_MODE_RING);
//				fwuliao3();
//				
//				if (anglea<179 && anglea>=0)
//					car(0,179,0);
//				else if( anglea>-179 && anglea <=0)
//					car(0,-179,0);
//				delay_ms(50);
//				//**************抓物料**************	
//				car(1,500,1850*4);
//				fnwuliao1();
//				zancun_na(1,500,1850*4,1);
//				fnwuliao2();
//				zancun_na(2,500,3900*4,2);
//				fnwuliao3();
//			}
//		}
//	}
//	
//	
//	if (saoma_data[4] == '3'){
//		car(21,500,1850*4);
//		vision(MAIXCAM_MODE_RING);
//		fwuliao1();
//		
//		if (saoma_data[5] == '2'){
//			car(22,500,1850*4);
//				fuwei();
//			vision(MAIXCAM_MODE_RING);
//			fwuliao2();
//			if (saoma_data[6] == '1'){
//				car(22,500,1850*4);
//				fuwei();
//				vision(MAIXCAM_MODE_RING);
//				fwuliao3();
//				
//				if (anglea<179 && anglea>=0)
//					car(0,179,0);
//				else if( anglea>-179 && anglea <=0)
//					car(0,-179,0);
//				delay_ms(50);
//				//**************抓物料**************	
//				car(1,500,3700*4);
//				fnwuliao1();
//				zancun_na(2,500,1850*4,1);
//				fnwuliao2();
//				zancun_na(2,500,1850*4,2);
//				fnwuliao3();
//			}
//		}
//		
//		if (saoma_data[5] == '1'){
//			car(22,500,3700*4);
//				fuwei();
//			vision(MAIXCAM_MODE_RING);
//			fwuliao2();
//			if (saoma_data[6] == '2'){
//				car(21,500,1850*4);
//				fuwei();
//				vision(MAIXCAM_MODE_RING);
//				fwuliao3();
//				
//				if (anglea<179 && anglea>=0)
//					car(0,179,0);
//				else if( anglea>-179 && anglea <=0)
//					car(0,-179,0);
//				delay_ms(50);
//				//**************抓物料**************	
//				car(1,500,1850*4);
//				fnwuliao1();
//				zancun_na(2,500,3700*4,1);
//				fnwuliao2();
//				zancun_na(1,500,1850*4,2);
//				fnwuliao3();
//			}
//		}
//	}
//	
	
}


void zancunqu2()
{
//	if (anglea<179 && anglea>=0)
//					car(0,179,0);
//				else if( anglea>-179 && anglea <=0)
//					car(0,-179,0);
//	delay_ms(50);
//	if (saoma_data[6] == '1'){
//		car(11,700,5870*4);
////		WL_3(8800*4);
//	}
//	else if (saoma_data[6] == '2'){
//		car(11,700,7725*4); 
////		WL_3(10650*4);
//	}
//	else if (saoma_data[6] == '3'){
//		car(11,700,9575*4);
////		WL_3(12500*4);
//	}
//	car(2,700,11000);
//	car(6,speed,4450*4);
//	car(0,90,0);
//	delay_ms(50);
//	car(2,speed,8000*4);

	car(0,-90,0);
	delay_ms(50);
	car(2,speed,11000*4);
	delay_ms(50);
		car(6,speed,4450*4);
		if (anglea<179 && anglea>=0)
					car(0,179,0);
				else if( anglea>-179 && anglea <=0)
					car(0,-179,0);
				delay_ms(50);
					car(2,speed,10000*4);
				delay_ms(50);
//	zhou(guiwei);    //夹子正对物料
//	car(0,90,0);
//	delay_ms(50);
//	

//		//***********放物料*************
//	if (saoma_data[4] == '1'){
//		car(2,500,1850*4);
//			fuwei();
//		vision(1);
//		maduo1();
//		
//		if (saoma_data[5] == '2'){
//			car(1,500,1850*4);
//				fuwei();
//			vision(2);
//			maduo2();
//			
//			if (saoma_data[6]=='3'){
//				car(1,500,1850*4);
//					fuwei();
//				vision(3);
//				maduo_z(fang3);

//			}
//			}
//		
//		if (saoma_data[5] == '3'){
//			car(1,500,3700*4);
//				fuwei();
//			vision(3);
//			maduo2();
//			if (saoma_data[6] == '2'){
//				car(2,500,1850*4);
//					fuwei();
//				vision(2);
//				maduo_z(fang3);
//				
//			}
//		}
//	}
//	
//	
//	if (saoma_data[4] == '2'){
//	fuwei();
//		vision(2);
//		maduo1();
//		
//		if (saoma_data[5] == '1'){
//			car(2,500,1850*4);
//				fuwei();
//			vision(1);
//			maduo_z(fang2);
//			if (saoma_data[6] == '3'){
//				car(1,500,3700*4);
//					fuwei();
//				vision(3);
//				maduo_z(fang3);
//				
//			}
//		}
//		
//		if (saoma_data[5] == '3'){
//			car(1,500,1850*4);
//				fuwei();
//			vision(3);
//			maduo2();
//			if (saoma_data[6] == '1'){
//				car(2,500,3700*4);
//					fuwei();
//				vision(1);
//				maduo3();
//				
//			}
//		}
//	}
//	
//	
//	if (saoma_data[4] == '3'){
//		car(1,500,1850*4);
//			fuwei();
//		vision(3);
//		maduo1();
//		
//		if (saoma_data[5] == '2'){
//			car(2,500,1850*4);
//				fuwei();
//			vision(2);
//			maduo_z(fang2);
//			if (saoma_data[6] == '1'){
//				car(2,500,1850*4);
//					fuwei();
//				vision(1);
//				maduo3();

//			}
//		}
//		
//		if (saoma_data[5] == '1'){
//			car(2,500,3700*4);
//				fuwei();
//			vision(1);
//			maduo2();
//			if (saoma_data[6] == '2'){
//				car(1,500,1850*4);
//					fuwei();
//				vision(2);
//				maduo_z(fang3);
//				
//			}
//		}
//	}
}


void qidian()
{
//		car(0,90,0);
//	delay_ms(50);
//	if (saoma_data[6] == '1'){
//		delay_ms(350);
//		zhou(fang1);
//		car(11,speed,6175*4);
//	}
//	else if (saoma_data[6] == '2'){
//		delay_ms(350);
//		zhou(fang1);
//		car(11,speed,8225*4);
//	}
//	else if (saoma_data[6] == '3'){
//		delay_ms(350);
//		zhou(fang1);
//		car(11,speed,10275*4);
//	}
//	car(6,speed,4450*4);
		if (anglea<179 && anglea>=0)
					car(0,179,0);
				else if( anglea>-179 && anglea <=0)
					car(0,-179,0);
				delay_ms(50);
    car(2,speed,11000*4);
				delay_ms(50);
			car(6,speed,4450*4);
	car(0,90,0);
	delay_ms(50);

//	car(0,0,0);
//	delay_ms(50);
	car(2,speed,22000*4);
				delay_ms(50);
				car(8,speed,4000*4);
	
	
//	if (qi=2)   // 检测到扫码完成
//    {
//			car(11,speed,1000*4);
//			car(2,speed,10000*4);
//			car(8,speed,4100*4);
//					
//    }
//	else{
//		car(8,speed,4100*4);
//}
}
