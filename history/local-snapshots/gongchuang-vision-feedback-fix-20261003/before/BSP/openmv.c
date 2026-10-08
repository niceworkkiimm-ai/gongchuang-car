#include "delay.h"
#include "Emm_V5.h"
#include "uart_4.h"
#include "uart_5.h"
#include "car.h"
#include "OLED.h"
#include "openmv.h"
#include "gyro_debug.h"
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
		Gyro_DisplayPoll(); /* Keep diagnostics live even when no movement branch runs. */

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



volatile uint8_t wl_wheel_wait_state = WL_WHEEL_IDLE;
volatile char wl_wheel_wait_axis = '-';

/* Cortex-M3 cycle counter: independent of delay_ms, which resets SysTick. */
#define WL_DWT_CTRL   (*(volatile uint32_t *)0xE0001000UL)
#define WL_DWT_CYCCNT (*(volatile uint32_t *)0xE0001004UL)
static uint8_t wl_align_started = 0;
static uint32_t wl_align_start_cycles = 0;
static uint32_t wl_align_budget_cycles = 0;

static void WL_StartDeadline(void)
{
    if (wl_align_started) return;
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    WL_DWT_CTRL |= 1UL;
    __DSB();
    __ISB();
    wl_align_budget_cycles = (SystemCoreClock / 1000UL) * WL_ALIGN_DELAY_MS;
    wl_align_start_cycles = WL_DWT_CYCCNT;
    wl_align_started = 1;
    wl_wheel_wait_state = WL_WHEEL_WAIT;
}

static int WL_DeadlineExpired(void)
{
    return wl_align_started &&
           (uint32_t)(WL_DWT_CYCCNT - wl_align_start_cycles) >= wl_align_budget_cycles;
}

static void WL_AlignDelay(uint32_t ms)
{
    while (ms-- && !WL_DeadlineExpired())
        delay_ms(1); /* Service vision/scanner/OLED; their time counts too. */
}

static void WL_FinishTimedAlignment(void)
{
    uint8_t addr;
    while (!WL_DeadlineExpired()) delay_ms(1);
    WheelReply_End();
    /* End wheel motion before the caller lowers the lift, with no ACK wait. */
    for (addr = 1; addr <= 4; ++addr)
    {
        Emm_V5_Stop_Now(addr, 0);
        delay_ms(10);
    }
    wl_wheel_wait_state = WL_WHEEL_DONE; /* Two seconds elapsed, not arrival. */
}

static void WL_MoveWheels(char axis, uint32_t pulses,
                          uint8_t d1, uint8_t d2, uint8_t d3, uint8_t d4)
{
    uint8_t addr;
    uint8_t dirs[4];
    uint32_t irq_mask;
    if (WL_DeadlineExpired()) return;
    dirs[0] = d1; dirs[1] = d2; dirs[2] = d3; dirs[3] = d4;
    wl_wheel_wait_axis = axis;
    /* Ignore replies left in the legacy buffer before this X command group. */
    if (axis == 'X')
    {
        irq_mask = __get_PRIMASK();
        __disable_irq();
        rxCmd[1] = rxCmd[2] = rxCmd[3] = 0;
        __set_PRIMASK(irq_mask);
    }
    WheelReply_Begin();
    for (addr = 1; addr <= 4 && !WL_DeadlineExpired(); ++addr)
    {
        WheelReply_Arm(addr);
        Emm_V5_Pos_Control(addr, dirs[addr - 1], 30, 0, pulses, 0, 0);
        WL_AlignDelay(10);
    }
    /* X: continue on one FD/9F/6B arrival, as in the original program.
     * Y: do not wait for a reply. The overall deadline remains unchanged. */
    if (axis == 'X')
    {
        while (!WL_DeadlineExpired())
        {
            if (rxCmd[1] == 0xFD && rxCmd[2] == 0x9F && rxCmd[3] == 0x6B)
            {
                irq_mask = __get_PRIMASK();
                __disable_irq();
                rxCmd[1] = rxCmd[2] = rxCmd[3] = 0;
                __set_PRIMASK(irq_mask);
                break;
            }
            WL_AlignDelay(1);
        }
    }
    WheelReply_End();
    WL_AlignDelay(50);
}

void WL_dingwei(char WL){
        wl_align_started = 0;
        WheelReply_End();
        wl_wheel_wait_state = WL_WHEEL_IDLE;
        wl_wheel_wait_axis = '-';
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
	
	
	int error_x2=0;
	int error_y2=0;
	int error_flag=0;
		
	WL_AlignDelay(20);
	while(flag_n<2){
			x = 0;
			y = 0;
			error_flag = 0;
	
	while(1){
        if (WL_DeadlineExpired()) goto alignment_finished;
//		pos_x=abs((int)round(error_x*0.5625*13.3333));
//		pos_y=abs((int)round(error_x*0.5625*13.3333));
		WL_AlignDelay(1); // Refresh diagnostics even before a target is found.
		if (!maixcam_found || x == 0 || y == 0){
			x = 0;
			y = 0;
		}
		else
		{
            WL_StartDeadline(); /* First recognized target starts the timer. */
			
				
			while(1){
        if (WL_DeadlineExpired()) goto alignment_finished;
//					if (!(x<60 && y<35) || !(x>180 && y<35)){		//×ªÅÌÆÁ±Î×óÉÏÓÒÉÏ
				error_flag++;
						WL_AlignDelay(70);
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
		

	WL_AlignDelay(50);

	if (error_x>5 || (error_x>5 && flag_n>0))
			{
				pos_x=abs((int)round(error_x*25.0));
				WL_AlignDelay(50);
				WL_MoveWheels('X', pos_x, 0, 1, 0, 1);


				}
			
			
				
			if (error_x<-5 || (error_x<-5 && flag_n>0))
			{
				pos_x=abs((int)round(error_x*25.0));
				WL_AlignDelay(50);
				WL_MoveWheels('X', pos_x, 1, 0, 1, 0);

		}
			
		WL_AlignDelay(50);
	
		if (error_y>5  || (error_y>5 && flag_n>0))
				{
					if (y<25){
						pos_y=abs((int)round(error_y*25.0*2));
					}
					else
						{
						pos_y=abs((int)round(error_y*25.0));
					}
					WL_AlignDelay(50);
					WL_MoveWheels('Y', pos_y, 0, 0, 1, 1);

							}

				
			
		

			
		
		if (error_y<-5 || (error_y<-5 && flag_n>0))
			{
				if (y<25){
						pos_y=abs((int)round(error_y*25.0*2));
					}
					else
						{
						pos_y=abs((int)round(error_y*25.0));
					}
				WL_AlignDelay(50);
				WL_MoveWheels('Y', pos_y, 1, 1, 0, 0);

			}
//		flag_a = 1;
		flag_n++;
		if (flag_n ==1){
			goto alignment_finished;
		}
		WL_AlignDelay(400);
			
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
	

alignment_finished:
    WL_FinishTimedAlignment();
}
