#include "delay.h"
#include "Emm_V5.h"
#include "uart_4.h"
#include "uart_5.h"
#include "car.h"
#include "OLED.h"
#include "openmv.h"
#include "gyro_debug.h"
#include "camera_debug.h"
#include "math.h"
#include <stdlib.h>
#include <wuliao.h>

volatile int error_x=0;
volatile int error_y=0;
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

#define VISION_FRAME_STALE_MS 1500UL
#define VISION_CENTER_TOLERANCE 5
#define VISION_LOCK_CONFIRM_FRAMES 3U

static void Vision_ReadCameraFeedback(uint32_t *frames, uint32_t *last_frame_ms,
                                      uint8_t *found, int *error_x_now,
                                      int *error_y_now, int custom_center,
                                      int center_x, int center_y)
{
    uint32_t irq_mask = __get_PRIMASK();
    __disable_irq();
    *frames = camera_debug_frames;
    *last_frame_ms = camera_debug_last_frame_ms;
    *found = maixcam_found;
    /* Compute from the same atomic camera snapshot as found/frame count. */
    *error_x_now = custom_center ? center_x - x : error_x;
    *error_y_now = custom_center ? center_y - y : error_y;
    __set_PRIMASK(irq_mask);
}

static void Vision_StopWheels(void)
{
    delay_ms(10);
    Emm_V5_Stop_Now(1, 0);
    delay_ms(10);
    Emm_V5_Stop_Now(2, 0);
    delay_ms(10);
    Emm_V5_Stop_Now(3, 0);
    delay_ms(10);
    Emm_V5_Stop_Now(4, 0);
}

static void Vision_SendWheelVelocity(uint8_t d1, uint8_t d2,
                                     uint8_t d3, uint8_t d4, int speed)
{
    delay_ms(10);
    Emm_V5_Vel_Control(1, d1, speed, speed, 0);
    delay_ms(10);
    Emm_V5_Vel_Control(2, d2, speed, speed, 0);
    delay_ms(10);
    Emm_V5_Vel_Control(3, d3, speed, speed, 0);
    delay_ms(10);
    Emm_V5_Vel_Control(4, d4, speed, speed, 0);
}

static void Vision_MoveX(int error, int speed)
{
    if (error > VISION_CENTER_TOLERANCE)
        Vision_SendWheelVelocity(0, 1, 0, 1, speed);
    else
        Vision_SendWheelVelocity(1, 0, 1, 0, speed);
}

static void Vision_MoveY(int error, int speed)
{
    if (error > VISION_CENTER_TOLERANCE)
        Vision_SendWheelVelocity(0, 0, 1, 1, speed);
    else
        Vision_SendWheelVelocity(1, 1, 0, 0, speed);
}

static void Vision_Run(char task, int custom_center, int center_x, int center_y)
{
    uint32_t frames, last_frame_ms, processed_frames;
    uint32_t now_ms, frame_age;
    uint8_t found, lock_frames = 0, wheel_velocity_active = 0;
    int error_x_now, error_y_now;

    chaoshi = 0;
    flag_n = 0;
    x = 0;
    y = 0;
    error_x = 0;
    error_y = 0;
    speed_x = 5;
    speed_y = 7;
    Usart_SendByte4(UART4, task);
    Vision_ReadCameraFeedback(&frames, &last_frame_ms, &found,
                              &error_x_now, &error_y_now, custom_center, center_x, center_y);
    processed_frames = frames;
    delay_ms(10);
    zhuazikai();

    while (1)
    {
        Vision_ReadCameraFeedback(&frames, &last_frame_ms, &found,
                                  &error_x_now, &error_y_now, custom_center, center_x, center_y);
        now_ms = CameraDebug_NowMs();
        frame_age = (uint32_t)(now_ms - last_frame_ms);

        /* Only issue a correction once for each newly validated camera frame. */
        if (frames == processed_frames)
        {
            if (wheel_velocity_active && frame_age >= VISION_FRAME_STALE_MS)
            {
                Vision_StopWheels();
                wheel_velocity_active = 0;
                lock_frames = 0;
            }
            Gyro_DisplayPoll();
            delay_ms(5);
            continue;
        }
        processed_frames = frames;

        /* A missing target or stale stream must never be treated as an offset. */
        if (!found || frame_age >= VISION_FRAME_STALE_MS)
        {
            if (wheel_velocity_active)
            {
                Vision_StopWheels();
                wheel_velocity_active = 0;
            }
            lock_frames = 0;
            Gyro_DisplayPoll();
            delay_ms(5);
            continue;
        }


        if (error_x_now >= -VISION_CENTER_TOLERANCE &&
            error_x_now <= VISION_CENTER_TOLERANCE &&
            error_y_now >= -VISION_CENTER_TOLERANCE &&
            error_y_now <= VISION_CENTER_TOLERANCE)
        {
            if (wheel_velocity_active || lock_frames == 0)
                Vision_StopWheels();
            wheel_velocity_active = 0;
            if (lock_frames < VISION_LOCK_CONFIRM_FRAMES)
                ++lock_frames;
            if (lock_frames >= VISION_LOCK_CONFIRM_FRAMES)
            {
                flag_n = 1;
                break;
            }
        }
        else
        {
            lock_frames = 0;
            flag_n = 1;
            /* Keep the original X-first correction order and motor directions. */
            if (error_x_now > VISION_CENTER_TOLERANCE ||
                error_x_now < -VISION_CENTER_TOLERANCE)
                Vision_MoveX(error_x_now, speed_x);
            else
                Vision_MoveY(error_y_now, speed_y);
            wheel_velocity_active = 1;
        }

        Gyro_DisplayPoll();
        delay_ms(5);
    }
}

/* Existing callers keep the alignment reference in uart_4.c. */
void vision(char task)
{
    Vision_Run(task, 0, 0, 0);
}

/* Use a caller-specific image point without changing material alignment. */
void vision_at(char task, int center_x, int center_y)
{
    Vision_Run(task, 1, center_x, center_y);
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

/* Poll only newly received, valid target frames. Waiting here does not use
 * the existing positioning budget, which starts only after this gate opens. */
static uint8_t WL_FirstTargetReady(uint32_t start_ms, uint32_t *seen_frames,
                                   uint8_t *skip_early)
{
    uint32_t irq_mask, frames, frame_ms, now_ms, frame_elapsed;
    uint8_t found;
    int image_x, image_y;
    irq_mask = __get_PRIMASK();
    __disable_irq();
    frames = camera_debug_frames;
    frame_ms = camera_debug_last_frame_ms;
    found = maixcam_found;
    image_x = x;
    image_y = y;
    __set_PRIMASK(irq_mask);
    if (frames == *seen_frames) return 0;
    *seen_frames = frames;
    now_ms = CameraDebug_NowMs();
    frame_elapsed = (uint32_t)(frame_ms - start_ms);
    if (!found || image_x <= 0 || image_x >= 320 ||
        image_y <= 0 || image_y >= 240 ||
        (uint32_t)(now_ms - frame_ms) >= VISION_FRAME_STALE_MS ||
        frame_elapsed > (uint32_t)(now_ms - start_ms))
        return 0;
    if (frame_elapsed <= WL_FIRST_EARLY_WINDOW_MS)
        *skip_early = 1;
    if (*skip_early && frame_elapsed < WL_FIRST_RETRY_AFTER_MS)
        return 0;
    return 1;
}

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

/* Read one complete camera frame; an absent target is never an offset.
 * Recovery waits use the remaining original two-second alignment window. */
static uint8_t WL_WaitValidCamera(int *error_x_now, int *error_y_now,
                                  int *image_y_now)
{
    uint32_t irq_mask, frames, last_frame_ms;
    uint8_t found;
    int image_x, image_y, ex, ey;
    while (!WL_DeadlineExpired())
    {
        irq_mask = __get_PRIMASK();
        __disable_irq();
        frames = camera_debug_frames;
        last_frame_ms = camera_debug_last_frame_ms;
        found = maixcam_found;
        image_x = x;
        image_y = y;
        ex = error_x;
        ey = error_y;
        __set_PRIMASK(irq_mask);
        if (frames && found && image_x > 0 && image_x < 320 &&
            image_y > 0 && image_y < 240 &&
            (uint32_t)(CameraDebug_NowMs() - last_frame_ms) < VISION_FRAME_STALE_MS)
        {
            *error_x_now = ex;
            *error_y_now = ey;
            *image_y_now = image_y;
            return 1;
        }
        WL_AlignDelay(1);
    }
    return 0;
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
        int align_error_x, align_error_y, align_image_y;
        uint32_t first_start_ms, first_seen_frames, first_irq_mask;
        uint8_t first_skip_early = 0;
        wl_align_started = 0;
        WheelReply_End();
        wl_wheel_wait_state = WL_WHEEL_IDLE;
        wl_wheel_wait_axis = '-';
        scanner_stage = 4;

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
		
        /* The caller has completed servo preparation. Ignore cached frames
         * from the previous mode, then start this visit's recognition clock. */
        first_irq_mask = __get_PRIMASK();
        __disable_irq();
        first_seen_frames = camera_debug_frames;
        first_start_ms = CameraDebug_NowMs();
        __set_PRIMASK(first_irq_mask);
        Usart_SendByte4(UART4,WL);
        scanner_stage = 5;
	WL_AlignDelay(20);
	while(flag_n<2){
			error_flag = 0;
	
	while(1){
        if (WL_DeadlineExpired()) goto alignment_finished;
//		pos_x=abs((int)round(error_x*0.5625*13.3333));
//		pos_y=abs((int)round(error_x*0.5625*13.3333));
		WL_AlignDelay(1); // Refresh diagnostics even before a target is found.
		if (!WL_FirstTargetReady(first_start_ms, &first_seen_frames,
                                    &first_skip_early))
            continue;
		{
            WL_StartDeadline(); /* Only an accepted target starts positioning. */
			
				
			while(1){
        if (WL_DeadlineExpired()) goto alignment_finished;
//					if (!(x<60 && y<35) || !(x>180 && y<35)){		//×ªÅÌÆÁ±Î×óÉÏÓÒÉÏ
				error_flag++;
						WL_AlignDelay(70);
                        if (!WL_WaitValidCamera(&align_error_x, &align_error_y,
                                                &align_image_y))
                            goto alignment_finished;
                        error_x2 = error_x2 + align_error_x;
                        error_y2 = error_y2 + align_error_y;
							if (error_flag == 5){
									error_x2 = error_x2 /5;
									error_y2 = error_y2 /5;

									if ((align_error_x == error_x2) && (align_error_y == error_y2)){
										
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
    if (!WL_WaitValidCamera(&align_error_x, &align_error_y, &align_image_y))
        goto alignment_finished;
    if (align_error_x > 5 || align_error_x < -5)
    {
        WL_AlignDelay(50);
        if (!WL_WaitValidCamera(&align_error_x, &align_error_y, &align_image_y))
            goto alignment_finished;
        pos_x = abs((int)round(align_error_x * 25.0));
        if (align_error_x > 5)
            WL_MoveWheels('X', pos_x, 0, 1, 0, 1);
        else if (align_error_x < -5)
            WL_MoveWheels('X', pos_x, 1, 0, 1, 0);
    }

    WL_AlignDelay(50);
    /* X motion can make the camera report found=0. Wait for valid Y rather
     * than converting that missing target (y=0) into 6250 movement pulses. */
    if (!WL_WaitValidCamera(&align_error_x, &align_error_y, &align_image_y))
        goto alignment_finished;
    if (align_error_y > 5 || align_error_y < -5)
    {
        WL_AlignDelay(50);
        if (!WL_WaitValidCamera(&align_error_x, &align_error_y, &align_image_y))
            goto alignment_finished;
        pos_y = abs((int)round(align_error_y * 25.0 *
                              (align_image_y < 25 ? 2.0 : 1.0)));
        if (align_error_y > 5)
            WL_MoveWheels('Y', pos_y, 0, 0, 1, 1);
        else if (align_error_y < -5)
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
