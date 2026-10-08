#include "delay.h"
#include "Emm_V5.h"
#include "uart_4.h"
#include "uart_5.h"
#include "car.h"
#include "OLED.h"
#include "openmv.h"
#include "math.h"
#include <stdlib.h>
#include <wuliao.h>

int error_x=0;
int error_y=0;
int speed_x=0;
int speed_y=0;
int chaoshi=0;

int flag_i=0;
int flag_n=0;

int error_x2=0;
int error_y2=0;
int error_flag=0;
int error_stop=0;
int delay_flag=0;
int flag_a=0;

int pos_x=0;
int pos_y=0;

int map_value(int x, int in_min, int in_max, int out_min, int out_max)
{
    if (x < in_min) x = in_min;
    if (x > in_max) x = in_max;
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void  vision(char task){
	
	
	Usart_SendByte4(UART4,task);
	
	

//	while(1){
//		if (x == 0 && y == 0){
//				x =0;
//			  y =0;
//		}
//		else
//		break;
//		
//	}
	chaoshi=0;
	flag_n=0;
	x=0;  
	y=0;
	error_x=0;
	error_y=0;
	
	speed_x=5;
	speed_y=7;
	delay_ms(10);
	zhuazikai();
	
	while(1){
//	chaoshi++;
	if (error_x>1)
			{
				flag_n = 1;
				delay_ms(10);
				Emm_V5_Vel_Control(1, 0, speed_x, speed_x,0);
				delay_ms(10);
				Emm_V5_Vel_Control(2, 1, speed_x, speed_x,0);
				delay_ms(10);
				Emm_V5_Vel_Control(3, 0, speed_x, speed_x,0);
				delay_ms(10);
				Emm_V5_Vel_Control(4, 1, speed_x, speed_x,0);
				delay_ms(100);
				
//					while (error_x>1)
//					{
////							error_x=120-x;
////							error_y=80-y;
//						if(error_x<=1){
//						delay_ms(10);
//						Emm_V5_Stop_Now(1,0);
//						delay_ms(10);
//						Emm_V5_Stop_Now(2,0);
//						delay_ms(10);
//						Emm_V5_Stop_Now(3,0);
//						delay_ms(10);
//						Emm_V5_Stop_Now(4,0);
//						}
//						}


				}
			

	
		if (error_y>1)
				{
					flag_n = 1;
					delay_ms(10);
					Emm_V5_Vel_Control(1, 0, speed_y, speed_y,0);
					delay_ms(10);
					Emm_V5_Vel_Control(2, 0, speed_y, speed_y,0);
					delay_ms(10);
					Emm_V5_Vel_Control(3, 1, speed_y, speed_y,0);
					delay_ms(10);
					Emm_V5_Vel_Control(4, 1, speed_y, speed_y,0);
					delay_ms(100);
//						while (error_y>1)
//						{
////							error_x=120-x;
////							error_y=80-y;
//							if (error_y<=1){
//							delay_ms(10);
//							Emm_V5_Stop_Now(1,0);
//							delay_ms(10);
//							Emm_V5_Stop_Now(2,0);
//							delay_ms(10);
//							Emm_V5_Stop_Now(3,0);
//							delay_ms(10);
//							Emm_V5_Stop_Now(4,0);
//							}
//								}

							}

				
			
		
	if (error_x<-1)
			{
				flag_n = 1;
				delay_ms(10);
				Emm_V5_Vel_Control(1, 1, speed_x, speed_x,0);
				delay_ms(10);
				Emm_V5_Vel_Control(2, 0, speed_x, speed_x,0);
				delay_ms(10);
				Emm_V5_Vel_Control(3, 1, speed_x, speed_x,0);
				delay_ms(10);
				Emm_V5_Vel_Control(4, 0, speed_x, speed_x,0);
			delay_ms(100);
//			while (error_x<-1)
//			{
////				error_x=120-x;
////				error_y=80-y;
//				if (error_x>=-1){
//				delay_ms(10);
//				Emm_V5_Stop_Now(1,0);
//				delay_ms(10);
//				Emm_V5_Stop_Now(2,0);
//				delay_ms(10);
//				Emm_V5_Stop_Now(3,0);
//				delay_ms(10);
//				Emm_V5_Stop_Now(4,0);
//				}
//			}

		}
			
		
		if (error_y<-1)
			{
				flag_n = 1;
				delay_ms(10);
				Emm_V5_Vel_Control(1, 1, speed_y, speed_y,0);
				delay_ms(10);
				Emm_V5_Vel_Control(2, 1, speed_y, speed_y,0);
				delay_ms(10);
				Emm_V5_Vel_Control(3, 0, speed_y, speed_y,0);
				delay_ms(10);
				Emm_V5_Vel_Control(4, 0, speed_y, speed_y,0);
				delay_ms(100);
//				while (error_y<-1)
//				{
////					error_x=120-x;
////					error_y=80-y;
//					if (error_y>=-1){
//					delay_ms(10);
//					Emm_V5_Stop_Now(1,0);
//					delay_ms(10);
//					Emm_V5_Stop_Now(2,0);
//					delay_ms(10);
//					Emm_V5_Stop_Now(3,0);
//					delay_ms(10);
//					Emm_V5_Stop_Now(4,0);
//						}
//					}

			}

			
		if (flag_n == 1 && error_x<=2 && error_x>=-2 && error_y<=2 && error_y>=-2){
					delay_ms(10);
					Emm_V5_Stop_Now(1,0);
					delay_ms(10);
					Emm_V5_Stop_Now(2,0);
					delay_ms(10);
					Emm_V5_Stop_Now(3,0);
					delay_ms(10);
					Emm_V5_Stop_Now(4,0);
					x=0;
					y=0;
					error_x=0;
					error_y=0;
//					Usart_SendByte4(UART4,0);
			break;
		}
		

	}
	
//	chaoshi=0;
	
}
 




void WL_dingwei2(char WL){
	
	Usart_SendByte4(UART4,WL);
	
	flag_n=0;
	x=0;  
	y=0;
	error_x=0;
	error_y=0;
	
	while(1){

	error_x2=0;
	error_y2=0;
	error_flag=0;
	error_stop=0;
	delay_flag=0;

		
	while(1){
			error_flag++;
			error_x2=error_x2+error_x;
			error_y2=error_y2+error_y;
			if (error_flag ==10){
				error_x2=error_x2/10;
				error_y2=error_y2/10;
				if (error_x==error_x2 && error_y== error_y2){
					error_stop=0;
					break;
				}
				delay_ms(10);
				Emm_V5_Stop_Now(1,0);
				delay_ms(10);
				Emm_V5_Stop_Now(2,0);
				delay_ms(10);
				Emm_V5_Stop_Now(3,0);
				delay_ms(10);
				Emm_V5_Stop_Now(4,0);
				delay_ms(20);
				error_x2=0;
				error_y2=0;
				error_flag=0;
			}
		}
	
	if (error_x>8 && error_stop==0)
			{
				flag_n = 1;
				delay_ms(10);
				Emm_V5_Vel_Control(1, 0, 15, 0,0);
				delay_ms(10);
				Emm_V5_Vel_Control(2, 1, 15, 0,0);
				delay_ms(10);
				Emm_V5_Vel_Control(3, 0, 15, 0,0);
				delay_ms(10);
				Emm_V5_Vel_Control(4, 1, 15, 0,0);
				delay_ms(100);
				
					while (error_x>1)
					{	
						delay_ms(10);
						delay_flag++;
						if (delay_flag == 20){
							break;
						}
						
						if(error_x<=1 || error_stop==1){
						delay_ms(10);
						Emm_V5_Stop_Now(1,0);
						delay_ms(10);
						Emm_V5_Stop_Now(2,0);
						delay_ms(10);
						Emm_V5_Stop_Now(3,0);
						delay_ms(10);
						Emm_V5_Stop_Now(4,0);
						}
						}


				}
			
				
		if (error_x<-8 && error_stop==0)
			{
				flag_n = 1;
				delay_ms(10);
				Emm_V5_Vel_Control(1, 1, 15, 0,0);
				delay_ms(10);
				Emm_V5_Vel_Control(2, 0, 15, 0,0);
				delay_ms(10);
				Emm_V5_Vel_Control(3, 1, 15, 0,0);
				delay_ms(10);
				Emm_V5_Vel_Control(4, 0, 15, 0,0);
			delay_ms(100);
			while (error_x<-1)
			{
				delay_ms(10);
				delay_flag++;
				if (delay_flag == 20){
					break;
				}
				
				
				
				if (error_x>=-1 || error_stop==1){
				delay_ms(10);
				Emm_V5_Stop_Now(1,0);
				delay_ms(10);
				Emm_V5_Stop_Now(2,0);
				delay_ms(10);
				Emm_V5_Stop_Now(3,0);
				delay_ms(10);
				Emm_V5_Stop_Now(4,0);
				}
			}
		}

		
			

		
		
	
		if (error_y>8 && error_stop==0)
				{
					flag_n = 1;
					delay_ms(10);
					Emm_V5_Vel_Control(1, 0, 15, 0,0);
					delay_ms(10);
					Emm_V5_Vel_Control(2, 0, 15, 0,0);
					delay_ms(10);
					Emm_V5_Vel_Control(3, 1, 15, 0,0);
					delay_ms(10);
					Emm_V5_Vel_Control(4, 1, 15, 0,0);
					delay_ms(100);
					
						while (error_y>1)
						{
							delay_ms(10);
							delay_flag++;
							if (delay_flag == 20){
								break;
							}
							
						
							if (error_y<=1 || error_stop==1){
							delay_ms(10);
							Emm_V5_Stop_Now(1,0);
							delay_ms(10);
							Emm_V5_Stop_Now(2,0);
							delay_ms(10);
							Emm_V5_Stop_Now(3,0);
							delay_ms(10);
							Emm_V5_Stop_Now(4,0);
							}
								}
							}

							

						
			
		
	
			
		
		if (error_y<-8 && error_stop==0)
			{
				flag_n = 1;
				delay_ms(10);
				Emm_V5_Vel_Control(1, 1, 15, 0,0);
				delay_ms(10);
				Emm_V5_Vel_Control(2, 1, 15, 0,0);
				delay_ms(10);
				Emm_V5_Vel_Control(3, 0, 15, 0,0);
				delay_ms(10);
				Emm_V5_Vel_Control(4, 0, 15, 0,0);
				delay_ms(100);
				
				
				while (error_y<-1)
				{
					
					delay_ms(10);
					delay_flag++;
					if (delay_flag == 20){
						break;
					}
						
					
				
				
					if (error_y>=-1 || error_stop==1){
					delay_ms(10);
					Emm_V5_Stop_Now(1,0);
					delay_ms(10);
					Emm_V5_Stop_Now(2,0);
					delay_ms(10);
					Emm_V5_Stop_Now(3,0);
					delay_ms(10);
					Emm_V5_Stop_Now(4,0);
						}
					}
				}

			
			
			
			
			if (flag_n == 1 && error_x<=8 && error_x>=-8 && error_y<=8 && error_y>=-8){
					delay_ms(10);
					Emm_V5_Stop_Now(1,0);
					delay_ms(10);
					Emm_V5_Stop_Now(2,0);
					delay_ms(10);
					Emm_V5_Stop_Now(3,0);
					delay_ms(10);
					Emm_V5_Stop_Now(4,0);
					x=0;
					y=0;
					error_x=0;
					error_y=0;
//					Usart_SendByte4(UART4,0);
					break;
		}
	
}

}



void WL_dingwei(char WL){
        scanner_stage = 4;
        Usart_SendByte4(UART4,WL);
        scanner_stage = 5;

	rxCmd[1] = 0x00;
	rxCmd[2] = 0x00;
	rxCmd[3] = 0x00;
	
//	flag_n=0;
//	x=0;  
//	y=0;
//	error_x=0;
//	error_y=0;
//	
	pos_x=0;
	pos_y=0;

//	
//	
//	while(1){
	flag_n=0;
	x=0;  
	y=0;
	error_x=0;
	error_y=0;
	flag_a=0;
	
	int time_i=0;
	
	int error_x2=0;
	int error_y2=0;
	int error_flag=0;
		
	delay_ms(20);
	while(flag_n<2){
			x = 0;
			y = 0;
			error_flag = 0;
	
	while(1){
//		pos_x=abs((int)round(error_x*0.5625*13.3333));
//		pos_y=abs((int)round(error_x*0.5625*13.3333));
		if (x == 0 || y == 0){
			x = 0;
			y = 0;
		}
		else
		{
			
				
			while(1){
//					if (!(x<60 && y<35) || !(x>180 && y<35)){		//×ªÅÌÆÁ±Î×óÉÏÓÒÉÏ
				error_flag++;
						delay_ms(70);
							error_x2 = error_x2+error_x;
							error_y2 = error_y2+error_y;
							if (error_flag == 5){
									error_x2 = error_x2 /5;
									error_y2 = error_y2 /5;

									if ((error_x == error_x2) && (error_y == error_y2)){
										
											break;
									}
	//						error_x2=error_x;
	//						error_y2=error_y;

							error_x2 = error_y2 = error_flag = 0;
								}    
//					}
			}
	
			break;
	}
		
		
		
}
		

	delay_ms(50);

	if (error_x>5 || (error_x>5 && flag_n>0))
			{
				pos_x=abs((int)round(error_x*0.5625*13.3333*4));
				delay_ms(50);
				Emm_V5_Pos_Control(1, 0, 30, 0, pos_x, 0, 0);
				delay_ms(10);
				Emm_V5_Pos_Control(2, 1, 30, 0, pos_x, 0, 0);
				delay_ms(10);
				Emm_V5_Pos_Control(3, 0, 30, 0, pos_x, 0, 0);
				delay_ms(10);
				Emm_V5_Pos_Control(4, 1, 30, 0, pos_x, 0, 0);
		//		delay_ms(10);
		//		Emm_V5_Synchronous_motion(0x00);
				while(1)
				{
					if (rxCmd[1] == 0xFD && rxCmd[2] == 0x9F && rxCmd [3] == 0x6B)
					{	
						rxCmd[1] = 0x00;
						rxCmd[2] = 0x00;
						rxCmd[3] = 0x00;
						delay_ms(50);
						break;
				}
					if (time_i==2000){
						time_i = 0;
						rxCmd[1] = 0x00;
						rxCmd[2] = 0x00;
						rxCmd[3] = 0x00;
						delay_ms(50);
						break;
					}
					time_i++;
					delay_ms(1);
				}


				}
			
			
				
			if (error_x<-5 || (error_x<-5 && flag_n>0))
			{
				pos_x=abs((int)round(error_x*0.5625*13.3333*4));
				delay_ms(50);
				Emm_V5_Pos_Control(1, 1, 30, 0, pos_x, 0, 0);
				delay_ms(10);
				Emm_V5_Pos_Control(2, 0, 30, 0, pos_x, 0, 0);
				delay_ms(10);
				Emm_V5_Pos_Control(3, 1, 30, 0, pos_x, 0, 0);
				delay_ms(10);
				Emm_V5_Pos_Control(4, 0, 30, 0, pos_x, 0, 0);
		//		delay_ms(10);
		//		Emm_V5_Synchronous_motion(0x00);
				while(1)
				{
					if (rxCmd[1] == 0xFD && rxCmd[2] == 0x9F && rxCmd [3] == 0x6B)
					{	
						rxCmd[1] = 0x00;
						rxCmd[2] = 0x00;
						rxCmd[3] = 0x00;
						delay_ms(50);
						break;
				}
					if (time_i==2000){
						time_i = 0;
						rxCmd[1] = 0x00;
						rxCmd[2] = 0x00;
						rxCmd[3] = 0x00;
						delay_ms(50);
						break;
					}
					time_i++;
					delay_ms(1);
				}

		}
			
		delay_ms(50);
	
		if (error_y>5  || (error_y>5 && flag_n>0))
				{
					if (y<25){
						pos_y=abs((int)round(error_y*0.5625*13.3333*4*2));
					}
					else
						{
						pos_y=abs((int)round(error_y*0.5625*13.3333*4));
					}
					delay_ms(50);
					Emm_V5_Pos_Control(1, 0, 30, 0, pos_y, 0, 0);
					delay_ms(10);
					Emm_V5_Pos_Control(2, 0, 30, 0, pos_y, 0, 0);
					delay_ms(10);
					Emm_V5_Pos_Control(3, 1, 30, 0, pos_y, 0, 0);
					delay_ms(10);
					Emm_V5_Pos_Control(4, 1, 30, 0, pos_y, 0, 0);
			//		delay_ms(10);
			//		Emm_V5_Synchronous_motion(0x00);
					while(1)
					{
						if (rxCmd[1] == 0xFD && rxCmd[2] == 0x9F && rxCmd [3] == 0x6B)
						{	
							rxCmd[1] = 0x00;
							rxCmd[2] = 0x00;
							rxCmd[3] = 0x00;
							delay_ms(50);
							break;
					}
						if (time_i==2000){
						time_i = 0;
						rxCmd[1] = 0x00;
						rxCmd[2] = 0x00;
						rxCmd[3] = 0x00;
						delay_ms(50);
						break;
					}
					time_i++;
					delay_ms(1);
					}

							}

				
			
		

			
		
		if (error_y<-5 || (error_y<-5 && flag_n>0))
			{
				if (y<25){
						pos_y=abs((int)round(error_y*0.5625*13.3333*4*2));
					}
					else
						{
						pos_y=abs((int)round(error_y*0.5625*13.3333*4));
					}
				delay_ms(50);
				Emm_V5_Pos_Control(1, 1, 30, 0, pos_y, 0, 0);
				delay_ms(10);
				Emm_V5_Pos_Control(2, 1, 30, 0, pos_y, 0, 0);
				delay_ms(10);
				Emm_V5_Pos_Control(3, 0, 30, 0, pos_y, 0, 0);
				delay_ms(10);
				Emm_V5_Pos_Control(4, 0, 30, 0, pos_y, 0, 0);
		//		delay_ms(10);
		//		Emm_V5_Synchronous_motion(0x00);
				while(1)
				{
					if (rxCmd[1] == 0xFD && rxCmd[2] == 0x9F && rxCmd [3] == 0x6B)
					{	
						rxCmd[1] = 0x00;
						rxCmd[2] = 0x00;
						rxCmd[3] = 0x00;
						delay_ms(50);
						break;
				}
					if (time_i==2000){
						time_i = 0;
						rxCmd[1] = 0x00;
						rxCmd[2] = 0x00;
						rxCmd[3] = 0x00;
						delay_ms(50);
						break;
					}
					time_i++;
					delay_ms(1);
				}

			}
//		flag_a = 1;
		flag_n++;
		if (flag_n ==1){
			return;
		}
		delay_ms(400);
			
		}
//		if (flag_n == 1 && error_x<=5 && error_x>=-5 && error_y<=5 && error_y>=-5){
//					delay_ms(10);
//					Emm_V5_Stop_Now(1,0);
//					delay_ms(10);
//					Emm_V5_Stop_Now(2,0);
//					delay_ms(10);
//					Emm_V5_Stop_Now(3,0);
//					delay_ms(10);
//					Emm_V5_Stop_Now(4,0);
//					x=0;
//					y=0;
//					error_x=0;
//					error_y=0;
//			
//					rxCmd[1] = 0x00;
//					rxCmd[2] = 0x00;
//					rxCmd[3] = 0x00;
////					Usart_SendByte4(UART4,0);
//			break;
//		}
//		

//	}
	

}
