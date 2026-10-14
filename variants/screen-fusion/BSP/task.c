#include "delay.h"
#include "Emm_V5.h"
#include "usart.h"
#include "car.h"
#include "openmv.h"
#include "wuliao.h"
#include "uart_5.h"
#include "uart_4.h"
#include "uart_3.h"
#include "camera_debug.h"
int speed_hua2=500;
int speed_hua_jia2=200;
int speed=500;
/* dir10路段：原直行值保留为基准，减去圆弧占用的距离。
 * 减一次TRIM=16998脉冲；两个弯之间的直行减两次。
 * car(10,speed,0)沿用原执行逻辑，脉冲按dir6标定换算；直线补偿需试车校验。
 */
extern volatile float anglea;

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
		
		Emm_V5_Pos_Control(5, 1, speed_hua2, speed_hua_jia2, 3500, 0, 0);    //升降台下降
		
	delay_ms(650);
	
		jiazi(fang);     //将物料2放进托盘
		
		Emm_V5_Pos_Control(5, 0, speed_hua2, speed_hua_jia2, 3500, 0, 0);      //升降台上升
	
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
		
		Emm_V5_Pos_Control(5, 1, speed_hua2, speed_hua_jia2, 3500, 0, 0);    //升降台下降
		
	 delay_ms(650);
	
		jiazi(fang);     //将物料2放进托盘
		
		Emm_V5_Pos_Control(5, 0, speed_hua2, speed_hua_jia2, 3500, 0, 0);      //升降台上升
	
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
		
	delay_ms(650);
	
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

void fuwei(){
	delay_ms(1); // Service foreground diagnostics while waiting for vision.
		x=0;
		y=0;
		error_x=0;
		error_y=0;
}





void ceshi3()
{
	car(9,speed,4000*4);//
			delay_ms(300);
	car(0,0,0);
	delay_ms(50);
	car(2,speed,11000*4);//前进
	car(0,0,0);
	delay_ms(50);
			car(6,speed,4450*4);//转90度
			delay_ms(400);
			car(0,-90,0);
	delay_ms(50);
			car(2,speed,12000*4);//直走
			//car(13,speed,4500*4);
	delay_ms(50);
		car(6,speed,4450*4);//转90度
	delay_ms(400);
			if (anglea<180 && anglea>=0)
					car(0,180,0);
				else if( anglea>-180 && anglea <=0)
					car(0,-180,0);
	delay_ms(100);
			car(2,speed,11500*4);
	delay_ms(50);			

	zhuazikai();
	zhuanpan0();
	delay_ms(300);
	fuwei();
	
	Scanner_WaitForData(); // 行走中持续接收扫码，到此还未收到则等待
	WL_dingwei(saoma_data[0]);
	delay_ms(50);
	wuliao1();
	fuwei();
	
	zhuazikai();

	if (saoma_data[1] == '1'){
		Usart_SendByte4(UART4,'7');
		while(1){
			fuwei();
		if (uart4_RxDataopenmv[0] == 0xa4){
			delay_ms(50);
			
			wuliao2();
			break;
		}
	}
	}
	else if (saoma_data[1] == '2'){
		Usart_SendByte4(UART4,'8');
		while(1){
			fuwei();
		if (uart4_RxDataopenmv[0] == 0xa5){
			delay_ms(50);
			
			wuliao2();
			break;
		}
	}
	}
	else if (saoma_data[1] == '3'){
		Usart_SendByte4(UART4,'9');
		while(1){
						fuwei();
		if (uart4_RxDataopenmv[0] == 0xa6){
			delay_ms(50);

			wuliao2();	
			break;
		}
	}
	}
	
	zhuazikai();
	
	if (saoma_data[2] == '1'){
		Usart_SendByte4(UART4,'7');
		while(1){
			fuwei();
		if (uart4_RxDataopenmv[0] == 0xa4){
			delay_ms(50);
			
			wuliao3();
			break;
		}
	}
	}
	else if (saoma_data[2] == '2'){
		Usart_SendByte4(UART4,'8');
		while(1){
			fuwei();
		if (uart4_RxDataopenmv[0] == 0xa5){
			delay_ms(50);
			
			wuliao3();
			break;
		}
	}
	}
	else if (saoma_data[2] == '3'){
		Usart_SendByte4(UART4,'9');
		while(1){
			fuwei();
		if (uart4_RxDataopenmv[0] == 0xa6){
			delay_ms(50);
			
			wuliao3();
			break;
		}
	}
	}
}
	
	
	
	
/* Wait for a fresh detection of a material color (camera modes 1..6). */
static void WaitMaterialColor(uint8_t color)
{
    uint8_t mode = (uint8_t)(color - '0');
    uint8_t command = mode <= 3 ? (uint8_t)('7' + mode - 1) : mode;
    uint8_t expected = mode <= 3 ? (uint8_t)(0xA3 + mode) : 0xA3;
    uint32_t first_frames = camera_debug_frames;

    Usart_SendByte4(UART4, command);
    while (1)
    {
        fuwei();
        if (camera_debug_frames != first_frames && maixcam_mode == mode &&
            maixcam_found && uart4_RxDataopenmv[0] == expected)
            return;
    }
}

void yuanliao()
{
	/* Wang Kai route: inter-station travel. */
	car(9,speed,4000*4);	//斜着走
	delay_ms(300);
	car(0,0,0);
	delay_ms(50);
	car(2,speed,52000);
	delay_ms(100);
	car(13,speed,51000-CAR_ARC_90_TRIM_PULSES); // 直行到扫码区并接转弯，合为一次运动
	//car(0,0,0);
	//delay_ms(200);
	// 原car(10,speed,0)已包含在dir12中，不再重复执行。
	//delay_ms(400);
	delay_ms(100);
	car(0,90,0);			//
	delay_ms(100);
	car(1,speed,32000);	//走到中心
	delay_ms(100);
	car(0,90,0);
	delay_ms(100);
	

	zhuazikai();
	zhuanpan0();
	delay_ms(100);
	fuwei();
	
	
	
	Scanner_WaitForData(); // 行走中持续接收扫码，到此还未收到则等待
	WL_dingwei(saoma_data[0]);
	delay_ms(50);
	
	
	
	wuliao1();
	fuwei();
	
	zhuazikai();

	WaitMaterialColor(saoma_data[1]);
	delay_ms(50);
	wuliao2();
	zhuazikai();

	WaitMaterialColor(saoma_data[2]);
	delay_ms(50);
	wuliao3();

}


void cujiagong()
{
    /* First coarse-processing round only; pixel X grows right, Y grows down. */
    const int ring_center_x = 148;
    const int ring_center_y = 128;

	/* Wang Kai route: inter-station travel. */
	car(6,speed,4500*4);
	delay_ms(200);
	car(0,0,0);
	car(2,speed,22250*4);
	delay_ms(50);
	zhou(guiwei);    //夹子正对物料
	Emm_V5_Pos_Control(5, 1, 500, 200, 8200, 0, 0);    //升降台降
	delay_ms(350);
	car(6,speed,4450*4);
	delay_ms(200);
	car(0,-90,0);	
	delay_ms(200);//gz

	


	//***********放物料*************
	/* At the rings: car(1) moves toward ring 1; car(2) toward ring 3. */
	if (saoma_data[6] == '1'){
		car(1,500,1850*4);
			fuwei();		//识别并且调整
		vision_at('4', ring_center_x, ring_center_y);
		fwuliao1();
		
		if (saoma_data[5] == '2'){
			car(2,500,1850*4);
				fuwei();
			vision_at('5', ring_center_x, ring_center_y);
			fwuliao2();
			
			if (saoma_data[4]=='3'){
				car(2,500,1850*4);
				fuwei();
				vision_at('6', ring_center_x, ring_center_y);
				fwuliao3();
				

				delay_ms(100);
			//**************抓物料**************	
				/* Already at ring 3: recover tray 1, then rings 2/1 for trays 2/3. */
				delay_ms(200);
				fnwuliao1();
				delay_ms(100);
				zancun_na(1,500,1850*4,1);
				fnwuliao2();
				//delay_ms(100);
				zancun_na(1,500,1850*4,2);
				fnwuliao3();
				car(2,500,1850*4); /* From ring 1 back to center ring 2. */
			}
			}
		
		if (saoma_data[5] == '3'){
			car(2,500,3700*4);
				fuwei();
			vision_at('6', ring_center_x, ring_center_y);
			fwuliao2();
			if (saoma_data[4] == '2'){
				car(1,500,1850*4);
				fuwei();
				vision_at('5', ring_center_x, ring_center_y);
				fwuliao3();
				
				delay_ms(100);
				//**************抓物料**************	
				/* Already at ring 2: recover tray 1, then rings 3/1 for trays 2/3. */
				delay_ms(200);
				fnwuliao1();
				delay_ms(100);
				zancun_na(2,500,1850*4,1);
				fnwuliao2();
				//delay_ms(100);
				zancun_na(1,500,3700*4,2);
				fnwuliao3();
				car(2,500,1850*4); /* From ring 1 back to center ring 2. */
			}
		}
	}
	
	
	if (saoma_data[6] == '2'){
			fuwei();
		vision_at('5', ring_center_x, ring_center_y);
		fwuliao1();
		
		if (saoma_data[5] == '1'){
			car(1,500,1850*4);
				fuwei();
			vision_at('4', ring_center_x, ring_center_y);
			fwuliao2();
			if (saoma_data[4] == '3'){
				
				car(2,500,3700*4);
				fuwei();
				vision_at('6', ring_center_x, ring_center_y);
				fwuliao3();
				
				delay_ms(100);
				//**************抓物料**************	
				/* Already at ring 3: recover tray 1, then rings 1/2 for trays 2/3. */
				delay_ms(200);
				fnwuliao1();
				delay_ms(100);
				zancun_na(1,500,3700*4,1);
				fnwuliao2();
				//delay_ms(100);
				zancun_na(2,500,1850*4,2);
				fnwuliao3();
				/* At center ring 2; ready for the next task. */
			}
		}
		
		if (saoma_data[5] == '3'){
			car(2,500,1850*4);
				fuwei();
			vision_at('6', ring_center_x, ring_center_y);
			fwuliao2();
			if (saoma_data[4] == '1'){
				car(1,500,3700*4);
				fuwei();
				vision_at('4', ring_center_x, ring_center_y);
				fwuliao3();
				
				delay_ms(100);
				//**************抓物料**************	
				/* Already at ring 1: recover tray 1, then rings 3/2 for trays 2/3. */
				delay_ms(200);
				fnwuliao1();
				delay_ms(100);
				zancun_na(2,500,3700*4,1);
				fnwuliao2();
				//delay_ms(100);
				zancun_na(1,500,1850*4,2);
				fnwuliao3();
				/* At center ring 2; ready for the next task. */
			}
		}
	}
	
	
	if (saoma_data[6] == '3'){
		car(2,500,1850*4);
		fuwei();
		vision_at('6', ring_center_x, ring_center_y);
		fwuliao1();
		
		if (saoma_data[5] == '2'){
			car(1,500,1850*4);
				fuwei();
			vision_at('5', ring_center_x, ring_center_y);
			fwuliao2();
			if (saoma_data[4] == '1'){
				car(1,500,1850*4);
				fuwei();
				vision_at('4', ring_center_x, ring_center_y);
				fwuliao3();
				
				delay_ms(100);
				//**************抓物料**************	
				/* Already at ring 1: recover tray 1, then rings 2/3 for trays 2/3. */
				delay_ms(200);
				fnwuliao1();
				delay_ms(100);
				zancun_na(2,500,1850*4,1);
				fnwuliao2();
				//delay_ms(100);
				zancun_na(2,500,1850*4,2);
				fnwuliao3();
				car(0,-90,0); /* Restore the coarse-area heading before returning. */
				car(1,500,1850*4); /* From ring 3 back to center ring 2. */
			}
		}
		
		if (saoma_data[5] == '1'){
			car(1,500,3700*4);
				fuwei();
			vision_at('4', ring_center_x, ring_center_y);
			fwuliao2();
			if (saoma_data[4] == '2'){
				car(2,500,1850*4);
				fuwei();
				vision_at('5', ring_center_x, ring_center_y);
				fwuliao3();
				
				delay_ms(100);
				//**************抓物料**************	
				/* Already at ring 2: recover tray 1, then rings 1/3 for trays 2/3. */
				delay_ms(200);
				fnwuliao1();
				delay_ms(100);
				zancun_na(1,500,1850*4,1);
				fnwuliao2();
				//delay_ms(100);
				zancun_na(2,500,3700*4,2);
				fnwuliao3();
				car(0,-90,0); /* Restore the coarse-area heading before returning. */
				car(1,500,1850*4); /* From ring 3 back to center ring 2. */
			}
		}
	}
	
	
	
	
}

void zancunqu()
{
    /* Same physical tray order as cujiagong: 3, 2, 1. */
    const int ring_center_x = 148;
    const int ring_center_y = 128;
	
	
	car(0,-90,0);
	delay_ms(200);
	
	car(12,speed,42000-CAR_ARC_90_TRIM_PULSES); 
	delay_ms(50);
	if (anglea<180 && anglea>=0)
		car(0,180,0);
	else if( anglea>-180 && anglea <=0)
		car(0,-180,0);
	car(2,speed,16000*2); 
	delay_ms(300);
	

//	/* Wang Kai route: inter-station travel. */
//	delay_ms(400);
//	car(0,-90,0);
//	delay_ms(50);
//	car(2,speed,11000*4-CAR_ARC_90_TRIM_PULSES);
//	delay_ms(50);
//	car(10,speed,0);
//	delay_ms(400);
//	if (anglea<180 && anglea>=0)
//		car(0,180,0);
//	else if( anglea>-180 && anglea <=0)
//		car(0,-180,0);
//	delay_ms(100);
//	car(2,speed,11000*4-CAR_ARC_90_TRIM_PULSES);
//	delay_ms(100);
	zhou(guiwei);    //夹子正对物料
	
	Emm_V5_Pos_Control(5, 1, 500, 200, 8200, 0, 0);
	delay_ms(650);

	/* Place tray 3 at scan[6], tray 2 at scan[5], tray 1 at scan[4]. */
	//***********放物料*************
	/* At the rings: car(1) moves toward ring 1; car(2) toward ring 3. */
	if (saoma_data[6] == '1'){
		car(1,500,1850*4);
			fuwei();		//识别并且调整
		vision_at('4', ring_center_x, ring_center_y);
		fwuliao1();
		
		if (saoma_data[5] == '2'){
			car(2,500,1850*4);
				fuwei();
			vision_at('5', ring_center_x, ring_center_y);
			fwuliao2();
			
			if (saoma_data[4]=='3'){
				car(2,500,1850*4);
				fuwei();
				vision_at('6', ring_center_x, ring_center_y);
				fwuliao3();
			}
			}
		
		if (saoma_data[5] == '3'){
			car(2,500,3700*4);
				fuwei();
			vision_at('6', ring_center_x, ring_center_y);
			fwuliao2();
			if (saoma_data[4] == '2'){
				car(1,500,1850*4);
				fuwei();
				vision_at('5', ring_center_x, ring_center_y);
				fwuliao3();
			}
		}
	}
	
	
	if (saoma_data[6] == '2'){
			fuwei();
		vision_at('5', ring_center_x, ring_center_y);
		fwuliao1();
		
		if (saoma_data[5] == '1'){
			car(1,500,1850*4);
				fuwei();
			vision_at('4', ring_center_x, ring_center_y);
			fwuliao2();
			if (saoma_data[4] == '3'){
				
				car(2,500,3700*4);
				fuwei();
				vision_at('6', ring_center_x, ring_center_y);
				fwuliao3();
			}
		}
		
		if (saoma_data[5] == '3'){
			car(2,500,1850*4);
				fuwei();
			vision_at('6', ring_center_x, ring_center_y);
			fwuliao2();
			if (saoma_data[4] == '1'){
				car(1,500,3700*4);
				fuwei();
				vision_at('4', ring_center_x, ring_center_y);
				fwuliao3();
			}
		}
	}
	
	
	if (saoma_data[6] == '3'){
		car(2,500,1850*4);
		fuwei();
		vision_at('6', ring_center_x, ring_center_y);
		fwuliao1();
		
		if (saoma_data[5] == '2'){
			car(1,500,1850*4);
				fuwei();
			vision_at('5', ring_center_x, ring_center_y);
			fwuliao2();
			if (saoma_data[4] == '1'){
				car(1,500,1850*4);
				fuwei();
				vision_at('4', ring_center_x, ring_center_y);
				fwuliao3();
			}
		}
		
		if (saoma_data[5] == '1'){
			car(1,500,3700*4);
				fuwei();
			vision_at('4', ring_center_x, ring_center_y);
			fwuliao2();
			if (saoma_data[4] == '2'){
				car(2,500,1850*4);
				fuwei();
				vision_at('5', ring_center_x, ring_center_y);
				fwuliao3();
			}
		}
	}
	
	
	
	
    /* Return to center ring 2 after the final placement at scan[4]. */
    if (saoma_data[4] == '1')
        car(2,500,1850*4); // 从1号圆环回到中间2号圆环
    else if (saoma_data[4] == '3')
        car(1,500,1850*4); // 从3号圆环回到中间2号圆环
    /* scan[4] == '2': already at center; no movement. */

}


//**************************************第二圈****************************************

void yuanliao2()
{
	/* Wang Kai route: inter-station travel. */
	delay_ms(350);
	zhou(fang1);
	if (anglea<180 && anglea>=0)
		car(0,180,0);
	else if( anglea>-180 && anglea <=0)
		car(0,-180,0);
	delay_ms(300);
	car(12,speed,44000-CAR_ARC_90_TRIM_PULSES);
	delay_ms(50);
	car(0,90,0);
	delay_ms(50);
	car(2,speed,31000);
	delay_ms(300);

	zhuazikai();
	zhuanpan0();
	delay_ms(300);
	fuwei();
	
	Scanner_WaitForData(); // 行走中持续接收扫码，到此还未收到则等待
	WL_dingwei(saoma_data[8]);
	delay_ms(50);
	wuliao1();
	fuwei();
	
	zhuazikai();

	WaitMaterialColor(saoma_data[9]);
	delay_ms(50);
	wuliao2();
	zhuazikai();

	WaitMaterialColor(saoma_data[10]);
	delay_ms(50);
	wuliao3();

}


void cujiagong2()
{
    const int ring_center_x = 148;
    const int ring_center_y = 128;
	/* Wang Kai route: inter-station travel. */
	car(6,speed,4500*4);
	delay_ms(300);
	car(0,0,0);
	car(2,speed,22250*4);
	delay_ms(50);
	zhou(guiwei);    //夹子正对物料
	Emm_V5_Pos_Control(5, 1, 500, 200, 8200, 0, 0);    //升降台降
	delay_ms(650);
	car(6,speed,4450*4);
	delay_ms(300);
	car(0,-90,0);

	//***********放物料*************
	/* At the rings: car(1) moves toward ring 1; car(2) toward ring 3. */
	if (saoma_data[14] == '1'){
		car(1,500,1850*4);
			fuwei();		//识别并且调整
		vision_at('4', ring_center_x, ring_center_y);
		fwuliao1();
		
		if (saoma_data[13] == '2'){
			car(2,500,1850*4);
				fuwei();
			vision_at('5', ring_center_x, ring_center_y);
			fwuliao2();
			
			if (saoma_data[12]=='3'){
				car(2,500,1850*4);
				fuwei();
				vision_at('6', ring_center_x, ring_center_y);
				fwuliao3();
				

				delay_ms(100);
			//**************抓物料**************	
				/* Already at ring 3: recover tray 1, then rings 2/1 for trays 2/3. */
				delay_ms(200);
				fnwuliao1();
				delay_ms(100);
				zancun_na(1,500,1850*4,1);
				fnwuliao2();
				delay_ms(200);
				zancun_na(1,500,1850*4,2);
				fnwuliao3();
				car(2,500,1850*4); /* From ring 1 back to center ring 2. */
			}
			}
		
		if (saoma_data[13] == '3'){
			car(2,500,3700*4);
				fuwei();
			vision_at('6', ring_center_x, ring_center_y);
			fwuliao2();
			if (saoma_data[12] == '2'){
				car(1,500,1850*4);
				fuwei();
				vision_at('5', ring_center_x, ring_center_y);
				fwuliao3();
				
				delay_ms(100);
				//**************抓物料**************	
				/* Already at ring 2: recover tray 1, then rings 3/1 for trays 2/3. */
				delay_ms(200);
				fnwuliao1();
				delay_ms(100);
				zancun_na(2,500,1850*4,1);
				fnwuliao2();
				delay_ms(200);
				zancun_na(1,500,3700*4,2);
				fnwuliao3();
				car(2,500,1850*4); /* From ring 1 back to center ring 2. */
			}
		}
	}
	
	
	if (saoma_data[14] == '2'){
			fuwei();
		vision_at('5', ring_center_x, ring_center_y);
		fwuliao1();
		
		if (saoma_data[13] == '1'){
			car(1,500,1850*4);
				fuwei();
			vision_at('4', ring_center_x, ring_center_y);
			fwuliao2();
			if (saoma_data[12] == '3'){
				
				car(2,500,3700*4);
				fuwei();
				vision_at('6', ring_center_x, ring_center_y);
				fwuliao3();
				
				delay_ms(100);
				//**************抓物料**************	
				/* Already at ring 3: recover tray 1, then rings 1/2 for trays 2/3. */
				delay_ms(200);
				fnwuliao1();
				delay_ms(100);
				zancun_na(1,500,3700*4,1);
				fnwuliao2();
				delay_ms(200);
				zancun_na(2,500,1850*4,2);
				fnwuliao3();
				/* At center ring 2; ready for the next task. */
			}
		}
		
		if (saoma_data[13] == '3'){
			car(2,500,1850*4);
				fuwei();
			vision_at('6', ring_center_x, ring_center_y);
			fwuliao2();
			if (saoma_data[12] == '1'){
				car(1,500,3700*4);
				fuwei();
				vision_at('4', ring_center_x, ring_center_y);
				fwuliao3();
				
				delay_ms(100);
				//**************抓物料**************	
				/* Already at ring 1: recover tray 1, then rings 3/2 for trays 2/3. */
				delay_ms(200);
				fnwuliao1();
				delay_ms(100);
				zancun_na(2,500,3700*4,1);
				fnwuliao2();
				delay_ms(200);
				zancun_na(1,500,1850*4,2);
				fnwuliao3();
				/* At center ring 2; ready for the next task. */
			}
		}
	}
	
	
	if (saoma_data[14] == '3'){
		car(2,500,1850*4);
		fuwei();
		vision_at('6', ring_center_x, ring_center_y);
		fwuliao1();
		
		if (saoma_data[13] == '2'){
			car(1,500,1850*4);
				fuwei();
			vision_at('5', ring_center_x, ring_center_y);
			fwuliao2();
			if (saoma_data[12] == '1'){
				car(1,500,1850*4);
				fuwei();
				vision_at('4', ring_center_x, ring_center_y);
				fwuliao3();
				
				delay_ms(100);
				//**************抓物料**************	
				/* Already at ring 1: recover tray 1, then rings 2/3 for trays 2/3. */
				delay_ms(200);
				fnwuliao1();
				delay_ms(100);
				zancun_na(2,500,1850*4,1);
				fnwuliao2();
				delay_ms(200);
				zancun_na(2,500,1850*4,2);
				fnwuliao3();
				car(1,500,1850*4); /* From ring 3 back to center ring 2. */
			}
		}
		
		if (saoma_data[13] == '1'){
			car(1,500,3700*4);
				fuwei();
			vision_at('4', ring_center_x, ring_center_y);
			fwuliao2();
			if (saoma_data[12] == '2'){
				car(2,500,1850*4);
				fuwei();
				vision_at('5', ring_center_x, ring_center_y);
				fwuliao3();
				
				delay_ms(100);
				//**************抓物料**************	
				/* Already at ring 2: recover tray 1, then rings 1/3 for trays 2/3. */
				delay_ms(200);
				fnwuliao1();
				delay_ms(100);
				zancun_na(1,500,1850*4,1);
				fnwuliao2();
				delay_ms(200);
				zancun_na(2,500,3700*4,2);
				fnwuliao3();
				car(1,500,1850*4); /* From ring 3 back to center ring 2. */
			}
		}
	}
	
	
	
	
}


/* Find the first-layer material color at a temporary-zone ring. */
static uint8_t first_round_color_for_ring(uint8_t ring)
{
    uint8_t i;
    for (i = 0; i < 3; ++i)
        if (saoma_data[4 + i] == ring) return saoma_data[i];
    return '0'; /* A validated ring permutation should never reach this. */
}

void zancunqu2()
{
    int tray, ring;
    int current_ring = 2;
    int target_rings[3] = {0, 0, 0};
    int exit_ring;
    /* Second-round stacking target in camera pixels: tune here only. */
    const int stack_center_x = 150;
    const int stack_center_y = 126;
	/* Wang Kai route: inter-station travel. */
	car(0,-90,0);
	delay_ms(200);
	
	car(12,speed,42000-CAR_ARC_90_TRIM_PULSES); 
	delay_ms(50);
	if (anglea<180 && anglea>=0)
		car(0,180,0);
	else if( anglea>-180 && anglea <=0)
		car(0,-180,0);
	car(2,speed,16250*2); 
	delay_ms(300);
	

	zhou(guiwei);    //夹子正对物料

		//***********放物料*************
    /* Third QR group gives tray 1/2/3 colors. Find each color in layer one. */
    for (tray = 0; tray < 3; ++tray)
    {
        for (ring = 1; ring <= 3; ++ring)
        {
            if (first_round_color_for_ring((uint8_t)('0' + ring)) ==
                saoma_data[8 + tray])
            {
                target_rings[tray] = ring;
                break;
            }
        }
        /* No same-color base exists: wait here instead of stacking elsewhere. */
        if (target_rings[tray] == 0)
            while (1) delay_ms(1);
    }

    /* Always return to center ring 2 after all three stacks. */
    exit_ring = 2;

    /* Pick trays 3 -> 2 -> 1, matching the requested reverse pickup order. */
    for (tray = 3; tray >= 1; --tray)
    {
        ring = target_rings[tray - 1];
        if (ring > current_ring)
            car(2, 500, (ring - current_ring) * 1850 * 4);
        else if (ring < current_ring)
            car(1, 500, (current_ring - ring) * 1850 * 4);
        current_ring = ring;

        fuwei();
        /* Raw modes 1..6 align to the same-color material in layer one. */
        vision_at((char)(saoma_data[7 + tray] - '0'),
                  stack_center_x, stack_center_y);
        if (tray == 3)
            maduo3();
        else if (tray == 2)
            maduo2();
        else
            maduo1();
    }

    /* Return from the last same-color stack to ring 2 before qidian(). */
    if (exit_ring > current_ring)
        car(2, 500, (exit_ring - current_ring) * 1850 * 4);
    else if (exit_ring < current_ring)
        car(1, 500, (current_ring - exit_ring) * 1850 * 4);
}


void qidian()
{
	/* Wang Kai route: inter-station travel. */
//	if (saoma_data[14] == '1'){
//		delay_ms(350);
//		zhou(fang1);
//	}
//	else if (saoma_data[14] == '2'){
//		delay_ms(350);
//		zhou(fang1);
//	}
//	else if (saoma_data[14] == '3'){
//		delay_ms(350);
//		zhou(fang1);
//	}
//	
	//调正
	
	delay_ms(100);
	car(12,speed,28000);
	delay_ms(200);
	car(0,90,0);
	delay_ms(50);
	car(2,speed,76000);
	delay_ms(200);
	car(0,90,0);
	delay_ms(50);
	car(8,speed,4000*4);

}

void ceshi2()
{
	zhuazikai();
	zhuanpan0();
	delay_ms(300);
	fuwei();
	
	Scanner_WaitForData(); // 行走中持续接收扫码，到此还未收到则等待
	WL_dingwei(saoma_data[0]);
	delay_ms(50);
	wuliao1();
	fuwei();
	
	zhuazikai();

	if (saoma_data[1] == '1'){
		Usart_SendByte4(UART4,'7');
		while(1){
			fuwei();
		if (uart4_RxDataopenmv[0] == 0xa4){
			delay_ms(50);
			
			wuliao2();
			break;
		}
	}
	}
	else if (saoma_data[1] == '2'){
		Usart_SendByte4(UART4,'8');
		while(1){
			fuwei();
		if (uart4_RxDataopenmv[0] == 0xa5){
			delay_ms(50);
			
			wuliao2();
			break;
		}
	}
	}
	else if (saoma_data[1] == '3'){
		Usart_SendByte4(UART4,'9');
		while(1){
						fuwei();
		if (uart4_RxDataopenmv[0] == 0xa6){
			delay_ms(50);

			wuliao2();	
			break;
		}
	}
	}
	
	zhuazikai();
	
	if (saoma_data[2] == '1'){
		Usart_SendByte4(UART4,'7');
		while(1){
			fuwei();
		if (uart4_RxDataopenmv[0] == 0xa4){
			delay_ms(50);
			
			wuliao3();
			break;
		}
	}
	}
	else if (saoma_data[2] == '2'){
		Usart_SendByte4(UART4,'8');
		while(1){
			fuwei();
		if (uart4_RxDataopenmv[0] == 0xa5){
			delay_ms(50);
			
			wuliao3();
			break;
		}
	}
	}
	else if (saoma_data[2] == '3'){
		Usart_SendByte4(UART4,'9');
		while(1){
			fuwei();
		if (uart4_RxDataopenmv[0] == 0xa6){
			delay_ms(50);
			
			wuliao3();
			break;
		}
	}
	}

}

void yuanhuanceshi(void )
{
	/* SIMULATED_SCAN_FOR_RING_TEST: equivalent to receiving 123+321. */
	uint32_t scan_test_irq_mask = __get_PRIMASK();
	__disable_irq();
	saoma_data[0] = '1';
	saoma_data[1] = '2';
	saoma_data[2] = '3';
	saoma_data[3] = '+';
	saoma_data[4] = '3';
	saoma_data[5] = '2';
	saoma_data[6] = '1';
	saoma_data[7] = '\0';
	saoma_ready = 1;
	scanner_stage = 3;
	__set_PRIMASK(scan_test_irq_mask);

		if (saoma_data[0] == '1'){
		car(1,500,1850*4);
			fuwei();		//识别并且调整
		vision('4');
		fwuliao1();
		
		if (saoma_data[1] == '2'){
			car(2,500,1850*4);
				fuwei();
			vision('5');
			fwuliao2();
			
			if (saoma_data[2]=='3'){
				car(2,500,1850*4);
				fuwei();
				vision('6');
				fwuliao3();
				
				if (anglea<180 && anglea>=0)
					car(0,180,0);
				else if( anglea>-180 && anglea <=0)
					car(0,-180,0);
				delay_ms(100);
			//**************抓物料**************	
				car(1,500,3700*4);
				fnwuliao1();
				zancun_na(2,500,1850*4,1);
				fnwuliao2();
				zancun_na(2,500,1850*4,2);
				fnwuliao3();
			}
			}
		
		if (saoma_data[1] == '3'){
			car(2,500,3700*4);
				fuwei();
			vision('6');
			fwuliao2();
			if (saoma_data[2] == '2'){
				car(1,500,1850*4);
				fuwei();
				vision('5');
				fwuliao3();
				
				if (anglea<180 && anglea>=0)
					car(0,180,0);
				else if( anglea>-180 && anglea <=0)
					car(0,-180,0);
				delay_ms(100);
				//**************抓物料**************	
				car(1,500,1850*4);
				fnwuliao1();
				zancun_na(2,500,3700*4,1);
				fnwuliao2();
				zancun_na(1,500,1850*4,2);
				fnwuliao3();
			}
		}
	}
	
	
	if (saoma_data[0] == '2'){
			fuwei();
		vision('5');
		fwuliao1();
		
		if (saoma_data[1] == '1'){
			car(1,500,1850*4);
				fuwei();
			vision('4');
			fwuliao2();
			if (saoma_data[2] == '3'){
				
				car(2,500,3700*4);
				fuwei();
				vision('6');
				fwuliao3();
				
				if (anglea<180 && anglea>=0)
					car(0,180,0);
				else if( anglea>-180 && anglea <=0)
					car(0,-180,0);
				delay_ms(100);
				//**************抓物料**************	
				car(1,500,1850*4);
				fnwuliao1();
				zancun_na(1,500,1850*4,1);
				fnwuliao2();
				zancun_na(2,500,3700*4,2);
				fnwuliao3();
			}
		}
		
		if (saoma_data[1] == '3'){
			car(2,500,1850*4);
				fuwei();
			vision('6');
			fwuliao2();
			if (saoma_data[2] == '1'){
				car(1,500,3700*4);
				fuwei();
				vision('4');
				fwuliao3();
				
				if (anglea<180 && anglea>=0)
					car(0,180,0);
				else if( anglea>-180 && anglea <=0)
					car(0,-180,0);
				delay_ms(100);
				//**************抓物料**************	
				car(2,500,1850*4);
				fnwuliao1();
				zancun_na(2,500,1850*4,1);
				fnwuliao2();
				zancun_na(1,500,3900*4,2);
				fnwuliao3();
			}
		}
	}
	
	
	if (saoma_data[0] == '3'){
		car(2,500,1850*4);
		fuwei();
		vision('6');
		fwuliao1();
		
		if (saoma_data[1] == '2'){
			car(1,500,1850*4);
				fuwei();
			vision('5');
			fwuliao2();
			if (saoma_data[2] == '1'){
				car(1,500,1850*4);
				fuwei();
				vision('4');
				fwuliao3();
				
				if (anglea<180 && anglea>=0)
					car(0,180,0);
				else if( anglea>-180 && anglea <=0)
					car(0,-180,0);
				delay_ms(100);
				//**************抓物料**************	
				car(2,500,3700*4);
				fnwuliao1();
				zancun_na(1,500,1850*4,1);
				fnwuliao2();
				zancun_na(1,500,1850*4,2);
				fnwuliao3();
			}
		}
		
		if (saoma_data[1] == '1'){
			car(1,500,3700*4);
				fuwei();
			vision('4');
			fwuliao2();
			if (saoma_data[2] == '2'){
				car(2,500,1850*4);
				fuwei();
				vision('5');
				fwuliao3();
				
				if (anglea<180 && anglea>=0)
					car(0,180,0);
				else if( anglea>-180 && anglea <=0)
					car(0,-180,0);
				delay_ms(100);
				//**************抓物料**************	
				car(2,500,1850*4);
				fnwuliao1();
				zancun_na(1,500,3700*4,1);
				fnwuliao2();
				zancun_na(2,500,1850*4,2);
				fnwuliao3();
			}
		}
	}
	
	
	
}




/* Coarse-processing test: restore each material to its original tray.
 * Placement: tray 3/2/1 -> scan[6]/[5]/[4]; retrieval reverses this order. */
void ceshi6(void)
{
	const int ring_center_x = 175;
    const int ring_center_y = 139;

    const char simulated_scan[] = "123+321";
    uint32_t irq_mask;
    unsigned int i;

    irq_mask = __get_PRIMASK();
    __disable_irq();
    for (i = 0; i < sizeof(simulated_scan); ++i)
        saoma_data[i] = (uint8_t)simulated_scan[i];
    saoma_ready = 1;
    __set_PRIMASK(irq_mask);

    Usart_SendByte4(UART4, 0x07); /* Raw numeric ring-recognition mode. */
	
	
	zhou(guiwei);   
	Emm_V5_Pos_Control(5, 1, 500, 200, 8200, 0, 0);    //升降台降	
	delay_ms(650);

	


	//***********放物料*************
	/* At the rings: car(1) moves toward ring 1; car(2) toward ring 3. */
	if (saoma_data[6] == '1'){
		car(1,500,1850*4);
			fuwei();		//识别并且调整
		vision_at('4', ring_center_x, ring_center_y);
		fwuliao1();
		
		if (saoma_data[5] == '2'){
			car(2,500,1850*4);
				fuwei();
			vision_at('5', ring_center_x, ring_center_y);
			fwuliao2();
			
			if (saoma_data[4]=='3'){
				car(2,500,1850*4);
				fuwei();
				vision_at('6', ring_center_x, ring_center_y);
				fwuliao3();
				

				delay_ms(100);
			//**************抓物料**************	
				/* Already at ring 3: recover tray 1, then rings 2/1 for trays 2/3. */
				delay_ms(200);
				fnwuliao1();
				delay_ms(100);
				zancun_na(1,500,1850*4,1);
				fnwuliao2();
				delay_ms(200);
				zancun_na(1,500,1850*4,2);
				fnwuliao3();
				car(2,500,1850*4); /* From ring 1 back to center ring 2. */
			}
			}
		
		if (saoma_data[5] == '3'){
			car(2,500,3700*4);
				fuwei();
			vision_at('6', ring_center_x, ring_center_y);
			fwuliao2();
			if (saoma_data[4] == '2'){
				car(1,500,1850*4);
				fuwei();
				vision_at('5', ring_center_x, ring_center_y);
				fwuliao3();
				
				delay_ms(100);
				//**************抓物料**************	
				/* Already at ring 2: recover tray 1, then rings 3/1 for trays 2/3. */
				delay_ms(200);
				fnwuliao1();
				delay_ms(100);
				zancun_na(2,500,1850*4,1);
				fnwuliao2();
				delay_ms(200);
				zancun_na(1,500,3700*4,2);
				fnwuliao3();
				car(2,500,1850*4); /* From ring 1 back to center ring 2. */
			}
		}
	}
	
	
	if (saoma_data[6] == '2'){
			fuwei();
		vision_at('5', ring_center_x, ring_center_y);
		fwuliao1();
		
		if (saoma_data[5] == '1'){
			car(1,500,1850*4);
				fuwei();
			vision_at('4', ring_center_x, ring_center_y);
			fwuliao2();
			if (saoma_data[4] == '3'){
				
				car(2,500,3700*4);
				fuwei();
				vision_at('6', ring_center_x, ring_center_y);
				fwuliao3();
				
				delay_ms(100);
				//**************抓物料**************	
				/* Already at ring 3: recover tray 1, then rings 1/2 for trays 2/3. */
				delay_ms(200);
				fnwuliao1();
				delay_ms(100);
				zancun_na(1,500,3700*4,1);
				fnwuliao2();
				delay_ms(200);
				zancun_na(2,500,1850*4,2);
				fnwuliao3();
				/* At center ring 2; ready for the next task. */
			}
		}
		
		if (saoma_data[5] == '3'){
			car(2,500,1850*4);
				fuwei();
			vision_at('6', ring_center_x, ring_center_y);
			fwuliao2();
			if (saoma_data[4] == '1'){
				car(1,500,3700*4);
				fuwei();
				vision_at('4', ring_center_x, ring_center_y);
				fwuliao3();
				
				delay_ms(100);
				//**************抓物料**************	
				/* Already at ring 1: recover tray 1, then rings 3/2 for trays 2/3. */
				delay_ms(200);
				fnwuliao1();
				delay_ms(100);
				zancun_na(2,500,3700*4,1);
				fnwuliao2();
				delay_ms(200);
				zancun_na(1,500,1850*4,2);
				fnwuliao3();
				/* At center ring 2; ready for the next task. */
			}
		}
	}
	
	
	if (saoma_data[6] == '3'){
		car(2,500,1850*4);
		fuwei();
		vision_at('6', ring_center_x, ring_center_y);
		fwuliao1();
		
		if (saoma_data[5] == '2'){
			car(1,500,1850*4);
				fuwei();
			vision_at('5', ring_center_x, ring_center_y);
			fwuliao2();
			if (saoma_data[4] == '1'){
				car(1,500,1850*4);
				fuwei();
				vision_at('4', ring_center_x, ring_center_y);
				fwuliao3();
				
				delay_ms(100);
				//**************抓物料**************	
				/* Already at ring 1: recover tray 1, then rings 2/3 for trays 2/3. */
				delay_ms(200);
				fnwuliao1();
				delay_ms(100);
				zancun_na(2,500,1850*4,1);
				fnwuliao2();
				delay_ms(200);
				zancun_na(2,500,1850*4,2);
				fnwuliao3();
				car(1,500,1850*4); /* From ring 3 back to center ring 2. */
			}
		}
		
		if (saoma_data[5] == '1'){
			car(1,500,3700*4);
				fuwei();
			vision_at('4', ring_center_x, ring_center_y);
			fwuliao2();
			if (saoma_data[4] == '2'){
				car(2,500,1850*4);
				fuwei();
				vision_at('5', ring_center_x, ring_center_y);
				fwuliao3();
				
				delay_ms(100);
				//**************抓物料**************	
				/* Already at ring 2: recover tray 1, then rings 1/3 for trays 2/3. */
				delay_ms(200);
				fnwuliao1();
				delay_ms(100);
				zancun_na(1,500,1850*4,1);
				fnwuliao2();
				delay_ms(200);
				zancun_na(2,500,3700*4,2);
				fnwuliao3();
				car(1,500,1850*4); /* From ring 3 back to center ring 2. */
			}
		}
	}
	
	
	
	
}

/* 仅站间行走测试：沿当前主流程走两轮后回起点，不扫码、不识别、不抓放。
 * car普通模式第三项为距离脉冲；car(0,角度,0)为陀螺仪目标朝向。
 * 不执行圆环左右移动，因此从区域到达点直接出发；暂存区离开位置与正式任务可能不同。
 * 本函数是独立路线副本：这里调距离不会自动改变主流程，调好后再同步对应任务函数。
 * 保留当前主流程已有的转向与调正；不自动添加用户已删除的调正。
 */
void xingzou(void)
{
    /* 第1段：起点 -> 扫码区 -> 第一轮原料区。对应 yuanliao()。 */
    car(9,speed,4000*4); // 斜走离开起点；距离调4000*4
    delay_ms(300);
    car(0,0,0); // 调正到0度
    delay_ms(50);
    car(2,speed,12000*4-CAR_ARC_90_TRIM_PULSES); // 走到扫码区；原直行基准调12000*4（本测试不等待扫码）
    car(0,0,0); // 调正到0度
    delay_ms(50);
    car(10,speed,0); // 扫码区后第一次弧线转弯；两侧脉冲8900/44500，前后直线已补偿
    delay_ms(400);
    car(0,-90,0); // 调正到-90度
    delay_ms(50);
    car(2,speed,11500*4-2*CAR_ARC_90_TRIM_PULSES); // 走向原料区方向的下一拐点；原直行基准调11500*4
    delay_ms(100);
    car(10,speed,0); // 第二次弧线转弯；两侧脉冲8900/44500，前后直线已补偿
    delay_ms(400);
    if (anglea<=180 && anglea>=0)
        car(0,180,0); // 调正到180度
    else if( anglea>=-180 && anglea <=0)
        car(0,-180,0); // 调正到-180度（与180度同一朝向）
    delay_ms(100);
    car(2,speed,12000*4-CAR_ARC_90_TRIM_PULSES); // 走向原料区；原直行基准调12000*4
    delay_ms(50);
    car(6,speed,4450*4); // 到原料区前最后一次原地转向
    delay_ms(400);
    car(0,90,0); // 调正到90度；到达第一轮原料区
    delay_ms(100);

    /* 第2段：第一轮原料区 -> 第一轮粗加工区。对应 cujiagong()。 */
    car(5,speed,4450*4); // 离开原料区，原地转向
    delay_ms(400);
    car(1,speed,11000*8); // 走向粗加工区；距离调11000*8，注意此处乘8
    delay_ms(1000);
    car(5,speed,4450*4); // 粗加工区前最后一次原地转向
    delay_ms(200);
    car(0,-90,0); // 调正到-90度；到达第一轮粗加工区

    /* 第3段：第一轮粗加工区 -> 第一轮暂存区。对应 zancunqu()。 */
    delay_ms(400);
    car(0,-90,0); // 离开粗加工区前调正到-90度
    delay_ms(50);
    car(2,speed,11500*4-CAR_ARC_90_TRIM_PULSES); // 走向暂存区方向的拐点；原直行基准调11500*4
    delay_ms(50);
    car(10,speed,0); // 拐点弧线转弯
    delay_ms(400);
    if (anglea<180 && anglea>=0)
        car(0,180,0); // 调正到180度
    else if( anglea>-180 && anglea <=0)
        car(0,-180,0); // 调正到-180度
    delay_ms(100);
    car(2,speed,11000*4-CAR_ARC_90_TRIM_PULSES); // 走到第一轮暂存区；原直行基准调11000*4
    delay_ms(100);

    /* 第4段：第一轮暂存区 -> 第二轮原料区。对应 yuanliao2()。 */
    delay_ms(350);
    if (anglea<180 && anglea>=0)
        car(0,180,0); // 离开暂存区前调正到180度
    else if( anglea>-180 && anglea <=0)
        car(0,-180,0); // 离开暂存区前调正到-180度
    delay_ms(300);
    car(2,speed,13000*4-CAR_ARC_90_TRIM_PULSES); // 走向第二轮原料区方向的拐点；原直行基准调13000*4
    delay_ms(300);
    if (anglea<180 && anglea>=0)
        car(0,180,0); // 转向前调正到180度
    else if( anglea>-180 && anglea <=0)
        car(0,-180,0); // 转向前调正到-180度
    delay_ms(100);
    car(10,speed,0); // 拐点弧线转弯
    delay_ms(400);
    car(0,90,0); // 调正到90度
    delay_ms(50);
    car(2,speed,11000*4-CAR_ARC_90_TRIM_PULSES); // 走到第二轮原料区；原直行基准调11000*4
    delay_ms(300);

    /* 第5段：第二轮原料区 -> 第二轮粗加工区。对应 cujiagong2()。 */
    car(6,speed,4500*4); // 离开第二轮原料区，原地转向；此处转向脉冲4500*4
    delay_ms(300);
    car(0,0,0); // 调正到0度
    car(2,speed,22500*4); // 走向第二轮粗加工区；距离调22500*4
    delay_ms(50);
    car(6,speed,4450*4); // 粗加工区前最后一次原地转向
    delay_ms(300);
    car(0,-90,0); // 调正到-90度；到达第二轮粗加工区

    /* 第6段：第二轮粗加工区 -> 第二轮暂存区。对应 zancunqu2()。 */
    delay_ms(300);
    car(0,-90,0); // 离开粗加工区前调正到-90度
    delay_ms(50);
    car(2,speed,11000*4-CAR_ARC_90_TRIM_PULSES); // 走向暂存区方向的拐点；原直行基准调11000*4
    delay_ms(400);
    car(10,speed,0); // 拐点弧线转弯
    delay_ms(300);
    if (anglea<180 && anglea>=0)
        car(0,180,0); // 调正到180度
    else if( anglea>-180 && anglea <=0)
        car(0,-180,0); // 调正到-180度
    delay_ms(100);
    car(2,speed,10000*4-CAR_ARC_90_TRIM_PULSES); // 走到第二轮暂存区；原直行基准调10000*4
    delay_ms(50);

    /* 第7段：第二轮暂存区 -> 返回起点。对应 qidian()。 */
    if (anglea<180 && anglea>=0)
        car(0,180,0); // 返程出发前调正到180度
    else if( anglea>-180 && anglea <=0)
        car(0,-180,0); // 返程出发前调正到-180度
    delay_ms(100);
    car(2,speed,11000*4-CAR_ARC_90_TRIM_PULSES); // 离开暂存区走向返程拐点；原直行基准调11000*4
    delay_ms(100);
    car(10,speed,0); // 返程拐点弧线转弯
    delay_ms(300);
    car(0,90,0); // 调正到90度
    delay_ms(50);
    car(2,speed,23000*4-CAR_ARC_90_TRIM_PULSES); // 走向起点入口；原直行基准调23000*4
    delay_ms(50);
    car(8,speed,4000*4); // 最后斜走回起点；距离调4000*4
}



void ceshi8(void)
{
	
	
	const int ring_center_x = 148;		//变大物料往左偏
	const int ring_center_y = 128;		//变大物料往上偏
	
	zhou(guiwei);   
	Emm_V5_Pos_Control(5, 1, 500, 200, 8200, 0, 0);    //升降台降	
	delay_ms(650);
	
	
	
	//***********放物料*************
	/* At the rings: car(1) moves toward ring 1; car(2) toward ring 3. */
	if (saoma_data[6] == '1'){
		car(1,500,1850*4);
			fuwei();		//识别并且调整
		vision_at('4', ring_center_x, ring_center_y);
		fwuliao1();
		
		if (saoma_data[5] == '2'){
			car(2,500,1850*4);
				fuwei();
			vision_at('5', ring_center_x, ring_center_y);
			fwuliao2();
			
			if (saoma_data[4]=='3'){
				car(2,500,1850*4);
				fuwei();
				vision_at('6', ring_center_x, ring_center_y);
				fwuliao3();
				

				delay_ms(100);
			//**************抓物料**************	
				/* Already at ring 3: recover tray 1, then rings 2/1 for trays 2/3. */
				delay_ms(200);
				fnwuliao1();
				delay_ms(100);
				zancun_na(1,500,1850*4,1);
				fnwuliao2();
				delay_ms(200);
				zancun_na(1,500,1850*4,2);
				fnwuliao3();
				car(2,500,1850*4); /* From ring 1 back to center ring 2. */
			}
			}
		
		if (saoma_data[5] == '3'){
			car(2,500,3700*4);
				fuwei();
			vision_at('6', ring_center_x, ring_center_y);
			fwuliao2();
			if (saoma_data[4] == '2'){
				car(1,500,1850*4);
				fuwei();
				vision_at('5', ring_center_x, ring_center_y);
				fwuliao3();
				
				delay_ms(100);
				//**************抓物料**************	
				/* Already at ring 2: recover tray 1, then rings 3/1 for trays 2/3. */
				delay_ms(200);
				fnwuliao1();
				delay_ms(100);
				zancun_na(2,500,1850*4,1);
				fnwuliao2();
				delay_ms(200);
				zancun_na(1,500,3700*4,2);
				fnwuliao3();
				car(2,500,1850*4); /* From ring 1 back to center ring 2. */
			}
		}
	}
	
	
	if (saoma_data[6] == '2'){
			fuwei();
		vision_at('5', ring_center_x, ring_center_y);
		fwuliao1();
		
		if (saoma_data[5] == '1'){
			car(1,500,1850*4);
				fuwei();
			vision_at('4', ring_center_x, ring_center_y);
			fwuliao2();
			if (saoma_data[4] == '3'){
				
				car(2,500,3700*4);
				fuwei();
				vision_at('6', ring_center_x, ring_center_y);
				fwuliao3();
				
				delay_ms(100);
				//**************抓物料**************	
				/* Already at ring 3: recover tray 1, then rings 1/2 for trays 2/3. */
				delay_ms(200);
				fnwuliao1();
				delay_ms(100);
				zancun_na(1,500,3700*4,1);
				fnwuliao2();
				delay_ms(200);
				zancun_na(2,500,1850*4,2);
				fnwuliao3();
				/* At center ring 2; ready for the next task. */
			}
		}
		
		if (saoma_data[5] == '3'){
			car(2,500,1850*4);
				fuwei();
			vision_at('6', ring_center_x, ring_center_y);
			fwuliao2();
			if (saoma_data[4] == '1'){
				car(1,500,3700*4);
				fuwei();
				vision_at('4', ring_center_x, ring_center_y);
				fwuliao3();
				
				delay_ms(100);
				//**************抓物料**************	
				/* Already at ring 1: recover tray 1, then rings 3/2 for trays 2/3. */
				delay_ms(200);
				fnwuliao1();
				delay_ms(100);
				zancun_na(2,500,3700*4,1);
				fnwuliao2();
				delay_ms(200);
				zancun_na(1,500,1850*4,2);
				fnwuliao3();
				/* At center ring 2; ready for the next task. */
			}
		}
	}
	
	
	if (saoma_data[6] == '3'){
		car(2,500,1850*4);
		fuwei();
		vision_at('6', ring_center_x, ring_center_y);
		fwuliao1();
		
		if (saoma_data[5] == '2'){
			car(1,500,1850*4);
				fuwei();
			vision_at('5', ring_center_x, ring_center_y);
			fwuliao2();
			if (saoma_data[4] == '1'){
				car(1,500,1850*4);
				fuwei();
				vision_at('4', ring_center_x, ring_center_y);
				fwuliao3();
				
				delay_ms(100);
				//**************抓物料**************	
				/* Already at ring 1: recover tray 1, then rings 2/3 for trays 2/3. */
				delay_ms(200);
				fnwuliao1();
				delay_ms(100);
				zancun_na(2,500,1850*4,1);
				fnwuliao2();
				delay_ms(200);
				zancun_na(2,500,1850*4,2);
				fnwuliao3();
				car(1,500,1850*4); /* From ring 3 back to center ring 2. */
			}
		}
		
		if (saoma_data[5] == '1'){
			car(1,500,3700*4);
				fuwei();
			vision_at('4', ring_center_x, ring_center_y);
			fwuliao2();
			if (saoma_data[4] == '2'){
				car(2,500,1850*4);
				fuwei();
				vision_at('5', ring_center_x, ring_center_y);
				fwuliao3();
				
				delay_ms(100);
				//**************抓物料**************	
				/* Already at ring 2: recover tray 1, then rings 1/3 for trays 2/3. */
				delay_ms(200);
				fnwuliao1();
				delay_ms(100);
				zancun_na(1,500,1850*4,1);
				fnwuliao2();
				delay_ms(200);
				zancun_na(2,500,3700*4,2);
				fnwuliao3();
				car(1,500,1850*4); /* From ring 3 back to center ring 2. */
			}
		}
	}
	
	
	
	
	
	
}

/* Test only: start at temporary-zone ring 2 with the second batch on trays.
 * First-layer materials must already be on their corresponding rings.
 * Uses the current scan data; no station travel or gyro heading correction.
 * Stop at the third stacking ring, without the formal return-to-ring-2 move.
 */
void ceshi9(void)
{
    int tray, ring;
    int current_ring = 2;
    int target_rings[3] = {0, 0, 0};
    /* Tune this test center independently; copy proven values to zancunqu2(). */
    const int stack_center_x = 150;
    const int stack_center_y = 126;

    zhou(guiwei);
    /* Map each second-batch tray color to its first-layer ring. */
    for (tray = 0; tray < 3; ++tray)
    {
        for (ring = 1; ring <= 3; ++ring)
        {
            if (first_round_color_for_ring((uint8_t)('0' + ring)) ==
                saoma_data[8 + tray])
            {
                target_rings[tray] = ring;
                break;
            }
        }
        if (target_rings[tray] == 0)
            while (1) delay_ms(1);
    }

    /* Tray 3 -> tray 2 -> tray 1; match each color with the first layer. */
    for (tray = 3; tray >= 1; --tray)
    {
        ring = target_rings[tray - 1];
        if (ring > current_ring)
            car(2, 500, (ring - current_ring) * 1850 * 4);
        else if (ring < current_ring)
            car(1, 500, (current_ring - ring) * 1850 * 4);
        current_ring = ring;

        fuwei();
        vision_at((char)(saoma_data[7 + tray] - '0'),
                  stack_center_x, stack_center_y);
        if (tray == 3)
            maduo3();
        else if (tray == 2)
            maduo2();
        else
            maduo1();
    }
}

/* Test only: place the car directly in front of physical temporary-zone ring 3.
 * Its first layer must already be present, and the second-batch trays loaded.
 * Stack the matching color once, return to ring 2, then run qidian().
 */
void ceshi10(void)
{
    int tray;
    uint8_t ring_color = first_round_color_for_ring('3');
    /* Keep this independent of the formal zancunqu2() camera center. */
    const int stack_center_x = 150;
    const int stack_center_y = 126;

    if (ring_color < '1' || ring_color > '6')
        while (1) delay_ms(1);

    for (tray = 1; tray <= 3; ++tray)
        if (saoma_data[7 + tray] == ring_color) break;
    if (tray > 3)
        while (1) delay_ms(1);

    zhou(guiwei);
    fuwei();
    vision_at((char)(ring_color - '0'), stack_center_x, stack_center_y);
    if (tray == 1)
        maduo1();
    else if (tray == 2)
        maduo2();
    else
        maduo3();

    car(1, 500, 1850 * 4); /* Physical ring 3 back to ring 2. */
    qidian();
}
