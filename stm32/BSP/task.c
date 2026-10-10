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
int speed_hua_jia2=200;
int speed=500;
extern float anglea;

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
		
		Emm_V5_Pos_Control(5, 1, speed_hua2, speed_hua_jia2, 4550, 0, 0);    //升降台下降
		
	delay_ms(650);
	
		jiazi(fang);     //将物料2放进托盘
		
		Emm_V5_Pos_Control(5, 0, speed_hua2, speed_hua_jia2, 4550, 0, 0);      //升降台上升
	
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
		
		Emm_V5_Pos_Control(5, 1, speed_hua2, speed_hua_jia2, 4550, 0, 0);    //升降台下降
		
	 delay_ms(650);
	
		jiazi(fang);     //将物料2放进托盘
		
		Emm_V5_Pos_Control(5, 0, speed_hua2, speed_hua_jia2, 4550, 0, 0);      //升降台上升
	
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
	
	
	
	
void yuanliao()
{
	/* Wang Kai route: inter-station travel. */
	car(9,speed,4000*4);	//斜着走
	delay_ms(300);
	car(0,0,0);
	delay_ms(50);
	car(2,speed,12000*4);	//走到扫码区
	car(0,0,0);
	delay_ms(50);
	car(6,speed,4450*4);	//原地转弯
	delay_ms(400);
	car(0,-90,0);			//
	delay_ms(50);
	car(2,speed,11500*4);	//走到中心
	delay_ms(100);
	car(6,speed,4450*4);	//原地转弯
	delay_ms(400);
	if (anglea<=180 && anglea>=0)
		car(0,180,0);
	else if( anglea>=-180 && anglea <=0)
		car(0,-180,0);
	delay_ms(100);
	car(2,speed,11200*4);
	delay_ms(50);
	
	car(6,speed,4450*4);
	delay_ms(400);
	car(0,90,0);
	delay_ms(100);		//转到物料区

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


void cujiagong()
{
    /* First coarse-processing round only; pixel X grows right, Y grows down. */
    const int ring_center_x = 175;
    const int ring_center_y = 139;

	/* Wang Kai route: inter-station travel. */
	
	car(5,speed,4450*4);
	delay_ms(400);
	
	if (anglea<180 && anglea>=0)
		car(0,180,0);
	else if( anglea>-180 && anglea <=0)
		car(0,-180,0);
	
	car(1,speed,11000*8);
	delay_ms(1000);
	
	if (anglea<180 && anglea>=0)
		car(0,180,0);
	else if( anglea>-180 && anglea <=0)
		car(0,-180,0);
	delay_ms(200);
	
	car(5,speed,4450*4);
	delay_ms(200);
	
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

void zancunqu()
{
    /* Same physical tray order as cujiagong: 3, 2, 1. */
    const int ring_center_x = 175;
    const int ring_center_y = 139;

	/* Wang Kai route: inter-station travel. */
	delay_ms(400);
	car(0,-90,0);
	delay_ms(50);
	car(2,speed,10000*4);
	delay_ms(50);
	car(6,speed,4450*4);
	delay_ms(400);
	if (anglea<180 && anglea>=0)
		car(0,180,0);
	else if( anglea>-180 && anglea <=0)
		car(0,-180,0);
	delay_ms(100);
	car(2,speed,11000*4);
	delay_ms(100);
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
	
	
	
	
}


//**************************************第二圈****************************************

void yuanliao2()
{
	/* Wang Kai route: inter-station travel. */
	if (saoma_data[2] == '1'){
		delay_ms(350);
		zhou(fang1);
	}
	else if (saoma_data[2] == '2'){
		delay_ms(350);
		zhou(fang1);
	}
	else if (saoma_data[2] == '3'){
		delay_ms(350);
		zhou(fang1);
	}
	if (anglea<180 && anglea>=0)
		car(0,180,0);
	else if( anglea>-180 && anglea <=0)
		car(0,-180,0);
	delay_ms(300);
	car(2,speed,11500*4);
	delay_ms(300);
	if (anglea<180 && anglea>=0)
		car(0,180,0);
	else if( anglea>-180 && anglea <=0)
		car(0,-180,0);
	delay_ms(100);
	car(6,speed,4450*4);
	delay_ms(400);
	car(0,90,0);
	delay_ms(50);
	car(2,speed,12000*4);
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

	if (saoma_data[9] == '1'){
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
	else if (saoma_data[9] == '2'){
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
	else if (saoma_data[9] == '3'){
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
	
	if (saoma_data[10] == '1'){
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
	else if (saoma_data[10] == '2'){
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
	else if (saoma_data[10] == '3'){
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


void cujiagong2()
{
	/* Wang Kai route: inter-station travel. */
	car(6,speed,4500*4);
	delay_ms(300);
	car(2,speed,22500*4);
	delay_ms(50);
	zhou(guiwei);    //夹子正对物料
	Emm_V5_Pos_Control(5, 1, speed_hua2, speed_hua_jia2, 6200*4, 0, 0);    //升降台降
	car(6,speed,4450*4);

	//***********放物料*************
	/* At the rings: car(1) moves toward ring 1; car(2) toward ring 3. */
	if (saoma_data[14] == '1'){
		car(1,500,1850*4);
			fuwei();		//识别并且调整
		vision('4');
		fwuliao1();
		
		if (saoma_data[13] == '2'){
			car(2,500,1850*4);
				fuwei();
			vision('5');
			fwuliao2();
			
			if (saoma_data[12]=='3'){
				car(2,500,1850*4);
				fuwei();
				vision('6');
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
			vision('6');
			fwuliao2();
			if (saoma_data[12] == '2'){
				car(1,500,1850*4);
				fuwei();
				vision('5');
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
		vision('5');
		fwuliao1();
		
		if (saoma_data[13] == '1'){
			car(1,500,1850*4);
				fuwei();
			vision('4');
			fwuliao2();
			if (saoma_data[12] == '3'){
				
				car(2,500,3700*4);
				fuwei();
				vision('6');
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
			vision('6');
			fwuliao2();
			if (saoma_data[12] == '1'){
				car(1,500,3700*4);
				fuwei();
				vision('4');
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
		vision('6');
		fwuliao1();
		
		if (saoma_data[13] == '2'){
			car(1,500,1850*4);
				fuwei();
			vision('5');
			fwuliao2();
			if (saoma_data[12] == '1'){
				car(1,500,1850*4);
				fuwei();
				vision('4');
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
			vision('4');
			fwuliao2();
			if (saoma_data[12] == '2'){
				car(2,500,1850*4);
				fuwei();
				vision('5');
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
	/* Wang Kai route: inter-station travel. */
	delay_ms(300);
	car(0,-90,0);
	delay_ms(50);
	car(2,speed,11000*4);
	delay_ms(400);
	car(6,speed,4450*4);
	delay_ms(300);
	if (anglea<180 && anglea>=0)
		car(0,180,0);
	else if( anglea>-180 && anglea <=0)
		car(0,-180,0);
	delay_ms(100);
	car(2,speed,10000*4);
	delay_ms(50);
	zhou(guiwei);    //夹子正对物料

		//***********放物料*************
	/* Placement order: QR digit 4 -> 5 -> 6, excluding the '+' separator. */
	if (saoma_data[12] == '1'){
		car(1,500,1850*4);
			fuwei();
		vision(first_round_color_for_ring('1'));
		maduo1();
		
		if (saoma_data[13] == '2'){
			car(2,500,1850*4);
				fuwei();
			vision(first_round_color_for_ring('2'));
			maduo2();
			
			if (saoma_data[14]=='3'){
				car(2,500,1850*4);
					fuwei();
				vision(first_round_color_for_ring('3'));
				maduo_z(fang3);

			}
			}
		
		if (saoma_data[13] == '3'){
			car(2,500,3700*4);
				fuwei();
			vision(first_round_color_for_ring('3'));
			maduo2();
			if (saoma_data[14] == '2'){
				car(1,500,1850*4);
					fuwei();
				vision(first_round_color_for_ring('2'));
				maduo_z(fang3);
				
			}
		}
	}
	
	
	if (saoma_data[12] == '2'){
	fuwei();
		vision(first_round_color_for_ring('2'));
		maduo1();
		
		if (saoma_data[13] == '1'){
			car(1,500,1850*4);
				fuwei();
			vision(first_round_color_for_ring('1'));
			maduo_z(fang2);
			if (saoma_data[14] == '3'){
				car(2,500,3700*4);
					fuwei();
				vision(first_round_color_for_ring('3'));
				maduo_z(fang3);
				
			}
		}
		
		if (saoma_data[13] == '3'){
			car(2,500,1850*4);
				fuwei();
			vision(first_round_color_for_ring('3'));
			maduo2();
			if (saoma_data[14] == '1'){
				car(1,500,3700*4);
					fuwei();
				vision(first_round_color_for_ring('1'));
				maduo3();
				
			}
		}
	}
	
	
	if (saoma_data[12] == '3'){
		car(2,500,1850*4);
			fuwei();
		vision(first_round_color_for_ring('3'));
		maduo1();
		
		if (saoma_data[13] == '2'){
			car(1,500,1850*4);
				fuwei();
			vision(first_round_color_for_ring('2'));
			maduo_z(fang2);
			if (saoma_data[14] == '1'){
				car(1,500,1850*4);
					fuwei();
				vision(first_round_color_for_ring('1'));
				maduo3();

			}
		}
		
		if (saoma_data[13] == '1'){
			car(1,500,3700*4);
				fuwei();
			vision(first_round_color_for_ring('1'));
			maduo2();
			if (saoma_data[14] == '2'){
				car(2,500,1850*4);
					fuwei();
				vision(first_round_color_for_ring('2'));
				maduo_z(fang3);
				
			}
		}
	}
}


void qidian()
{
	/* Wang Kai route: inter-station travel. */
	if (saoma_data[14] == '1'){
		delay_ms(350);
		zhou(fang1);
	}
	else if (saoma_data[14] == '2'){
		delay_ms(350);
		zhou(fang1);
	}
	else if (saoma_data[14] == '3'){
		delay_ms(350);
		zhou(fang1);
	}
	if (anglea<180 && anglea>=0)
		car(0,180,0);
	else if( anglea>-180 && anglea <=0)
		car(0,-180,0);
	delay_ms(100);
	car(2,speed,11000*4);
	delay_ms(100);
	car(6,speed,4450*4);
	delay_ms(300);
	car(0,90,0);
	delay_ms(50);
	car(2,speed,23000*4);
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
