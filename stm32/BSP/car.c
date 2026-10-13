#include "delay.h"
#include "Emm_V5.h"
#include "usart.h"
#include "gyro_debug.h"
#include "car.h"
#include "camera_debug.h"

volatile float anglea=0;
/* Observation only: these values never select or stop motor commands. */
volatile int16_t car_debug_mode = -1;
volatile int16_t car_debug_target = 0;
volatile uint8_t car_debug_active = 0;

int speed_add=150;

extern volatile float global_angle;

/* Keep fractional yaw. Arrival uses the shortest wrapped angular error. */
#define CAR_HEADING_TOLERANCE_DEG 0.5f
static uint8_t Car_HeadingReached(float target)
{
    float error = target - anglea;
    if (error > 180.0f) error -= 360.0f;
    else if (error < -180.0f) error += 360.0f;
    return error >= -CAR_HEADING_TOLERANCE_DEG &&
           error <= CAR_HEADING_TOLERANCE_DEG;
}

volatile uint8_t car_arc_fault = 0;
volatile uint8_t car_arc_fault_addr = 0;

static void CarArc_HoldFault(uint8_t code, uint8_t addr)
{
    uint8_t motor;
    car_arc_fault = code;
    car_arc_fault_addr = addr;
    WheelStatus_End();
    for (motor = 1; motor <= 4; ++motor)
    {
        Emm_V5_Stop_Now(motor, 0);
        delay_ms(10);
    }
    /* A failed arc must not continue into the following straight. */
    for (;;)
    {
        Gyro_DisplayPoll();
        delay_ms(10);
    }
}

static uint8_t CarArc_ReadFlags(uint8_t addr)
{
    uint8_t attempt, flags;
    uint32_t start;
    for (attempt = 0; attempt < 3; ++attempt)
    {
        WheelStatus_Begin(addr);
        Emm_V5_Read_Sys_Params(addr, S_FLAG);
        start = CameraDebug_NowMs();
        while (!wheel_status_ready &&
               (uint32_t)(CameraDebug_NowMs() - start) < 100UL)
        {
            Gyro_DisplayPoll();
            delay_ms(1);
        }
        if (wheel_status_ready)
        {
            flags = wheel_status_flags;
            WheelStatus_End();
            return flags;
        }
        WheelStatus_End();
        delay_ms(10);
    }
    CarArc_HoldFault(1, addr);
    return 0;
}

static void CarArc_WaitAll(uint8_t require_motion, uint32_t timeout_ms)
{
    uint8_t addr, flags, bit, done, moved = require_motion ? 0 : 0x0F;
    uint32_t start = CameraDebug_NowMs();
    do
    {
        done = 0;
        for (addr = 1; addr <= 4; ++addr)
        {
            flags = CarArc_ReadFlags(addr);
            if (!(flags & 0x01) || (flags & 0x0C))
                CarArc_HoldFault(2, addr);
            bit = (uint8_t)(1U << (addr - 1));
            if (!(flags & 0x02)) moved |= bit;
            else if (moved & bit) done |= bit;
        }
        if (done == 0x0F) return;
        if ((uint32_t)(CameraDebug_NowMs() - start) >= timeout_ms)
            CarArc_HoldFault(3, 0);
        delay_ms(5);
    } while (1);
}

static void Car_Arc90(int requested_speed)
{
    uint8_t addr, inner;
    uint16_t outer_speed, inner_speed;
    uint32_t timeout_ms;
    car_arc_fault = 0;
    car_arc_fault_addr = 0;
    if (requested_speed < 5) CarArc_HoldFault(4, 0);
    /* Keep the old dir10 maximum of 500 RPM; exact 5:1 speed ratio. */
    if (requested_speed > 500) requested_speed = 500;
    outer_speed = (uint16_t)((requested_speed / 5) * 5);
    inner_speed = outer_speed / 5;
    timeout_ms = 15000UL * 500UL / outer_speed;

    /* Legacy straight moves return on the first completion. Wait for all. */
    CarArc_WaitAll(0, timeout_ms);
    for (addr = 1; addr <= 4; ++addr)
    {
        inner = (addr == 1 || addr == 3);
        /* Emm ramp time = RPM*(256-acc)*50us.
         * (256-151)*inner_speed == (256-235)*outer_speed.
         * Both paths have equal duration, including acceleration/deceleration.
         */
        Emm_V5_Pos_Control(addr, inner ? 1 : 0,
                           inner ? inner_speed : outer_speed,
                           inner ? 151 : 235,
                           inner ? CAR_ARC_90_INNER_PULSES : CAR_ARC_90_OUTER_PULSES,
                           0, 1);
        delay_ms(10);
    }
    Emm_V5_Synchronous_motion(0);
    /* Observe each wheel moving, then reached. An old reached bit is not enough.
     * Poll individually: simultaneous unsolicited arrival frames may collide.
     */
    CarArc_WaitAll(1, timeout_ms);
    delay_ms(20);
}

void car(int dir,int speed,int location)
{
    car_debug_mode = (int16_t)dir;
    car_debug_target = dir == 0 ? (int16_t)speed : 0;
    car_debug_active = 1;
	while ((rxCmd[1] != 0x00) || (rxCmd[2] != 0x00) || (rxCmd[3] != 0x00))
	{

		rxCmd[1] = 0x00;
		rxCmd[2] = 0x00;
		rxCmd[3] = 0x00;
	}

	if (dir == 0)
	{

			if (!Car_HeadingReached((float)speed) &&
                ((anglea < speed) || ((anglea > speed) && (anglea >135))))
			{
				delay_ms(10);
				Emm_V5_Vel_Control(1, 1, 10, 10,0);
				delay_ms(10);
				Emm_V5_Vel_Control(2, 1, 10, 10,0);
				delay_ms(10);
				Emm_V5_Vel_Control(3, 1, 10, 10,0);
				delay_ms(10);
				Emm_V5_Vel_Control(4, 1, 10, 10,0);

			}
			if (!Car_HeadingReached((float)speed) &&
                ((anglea > speed) || ((anglea < speed) && (anglea <-135))))
			{
				delay_ms(10);
				Emm_V5_Vel_Control(1, 0, 10, 10,0);
				delay_ms(10);
				Emm_V5_Vel_Control(2, 0, 10, 10,0);
				delay_ms(10);
				Emm_V5_Vel_Control(3, 0, 10, 10,0);
				delay_ms(10);
				Emm_V5_Vel_Control(4, 0, 10, 10,0);

			}
		while (1){
			if (Car_HeadingReached((float)speed))
			{
				delay_ms(10);
				Emm_V5_Stop_Now(1,0);
				delay_ms(10);
				Emm_V5_Stop_Now(2,0);
				delay_ms(10);
				Emm_V5_Stop_Now(3,0);
				delay_ms(10);
				Emm_V5_Stop_Now(4,0);
				break;
			}
			Gyro_DisplayPoll(); /* Keep OLED live while waiting. */
		}
	}
		
	if (dir == 1) 
	{
		delay_ms(10);
		Emm_V5_Pos_Control(1, 0, speed, speed_add, location, 0, 0);
		delay_ms(10);
		Emm_V5_Pos_Control(2, 1, speed, speed_add, location, 0, 0);
		delay_ms(10);
		Emm_V5_Pos_Control(3, 0, speed, speed_add, location, 0, 0);
		delay_ms(10);
		Emm_V5_Pos_Control(4, 1, speed, speed_add, location, 0, 0);
//		delay_ms(10);
//		Emm_V5_Synchronous_motion(0x00);
		while(1)
		{
			if (rxCmd[1] == 0xFD && rxCmd[2] == 0x9F && rxCmd [3] == 0x6B)
			{	
				delay_ms(20);
				break;
		}
			Gyro_DisplayPoll(); /* Keep OLED live while waiting. */
		}
	}
	
	if (dir == 2)
	{
		delay_ms(10);
		Emm_V5_Pos_Control(1, 1, speed, speed_add, location, 0, 0);
		delay_ms(10);
		Emm_V5_Pos_Control(2, 0, speed, speed_add, location, 0, 0);
		delay_ms(10);
		Emm_V5_Pos_Control(3, 1, speed, speed_add, location, 0, 0);
		delay_ms(10);
		Emm_V5_Pos_Control(4, 0, speed, speed_add, location, 0, 0);
//		delay_ms(10);
//		Emm_V5_Synchronous_motion(0x00);
		while(1)
		{
			if (rxCmd[1] == 0xFD && rxCmd[2] == 0x9F && rxCmd [3] == 0x6B)
			{	
				delay_ms(20);
				break;
		}
			Gyro_DisplayPoll(); /* Keep OLED live while waiting. */
		}
		
	}
	
	
	
		//*********粗加工物料专用********
	
		if (dir == 21) 
	{
		delay_ms(10);
		Emm_V5_Pos_Control(1, 0, speed, speed_add, location, 0, 0);
		delay_ms(10);
		Emm_V5_Pos_Control(2, 1, speed, speed_add, location, 0, 0);
		delay_ms(10);
		Emm_V5_Pos_Control(3, 0, speed, speed_add, location, 0, 0);
		delay_ms(10);
		Emm_V5_Pos_Control(4, 1, speed, speed_add, location, 0, 0);
//		delay_ms(10);
//		Emm_V5_Synchronous_motion(0x00);
		uint32_t timeout = 0;   // 计时器，单位ms
				
		while(1)
		{
		// 每次循环延时一点，避免CPU卡死
        delay_ms(10);
        timeout += 10;

        // 超时处理
        if (timeout >= 3000)
        {
            // 这里可以做强制停止或报错处理
            Emm_V5_Stop_Now(1, 0);
            Emm_V5_Stop_Now(2, 0);
            Emm_V5_Stop_Now(3, 0);
            Emm_V5_Stop_Now(4, 0);
            break;
        }
				
			if (rxCmd[1] == 0xFD && rxCmd[2] == 0x9F && rxCmd [3] == 0x6B)
			{	
				delay_ms(20);
				break;
		}
			Gyro_DisplayPoll(); /* Keep OLED live while waiting. */
		}
	}
	
	if (dir == 22)
	{
		delay_ms(10);
		Emm_V5_Pos_Control(1, 1, speed, speed_add, location, 0, 0);
		delay_ms(10);
		Emm_V5_Pos_Control(2, 0, speed, speed_add, location, 0, 0);
		delay_ms(10);
		Emm_V5_Pos_Control(3, 1, speed, speed_add, location, 0, 0);
		delay_ms(10);
		Emm_V5_Pos_Control(4, 0, speed, speed_add, location, 0, 0);
		
		uint32_t timeout = 0;   // 计时器，单位ms
//		delay_ms(10);
//		Emm_V5_Synchronous_motion(0x00);
		while(1)
		{
			
					// 每次循环延时一点，避免CPU卡死
        delay_ms(10);
        timeout += 10;

        // 超时处理
        if (timeout >= 3000)
        {
            // 这里可以做强制停止或报错处理
            Emm_V5_Stop_Now(1, 0);
            Emm_V5_Stop_Now(2, 0);
            Emm_V5_Stop_Now(3, 0);
            Emm_V5_Stop_Now(4, 0);
            break;
        }
			
			if (rxCmd[1] == 0xFD && rxCmd[2] == 0x9F && rxCmd [3] == 0x6B)
			{	
				delay_ms(20);
				break;
		}
			Gyro_DisplayPoll(); /* Keep OLED live while waiting. */
		}
		
	}
	
	
	
	
	if (dir == 3)
	{
		delay_ms(10);
		Emm_V5_Pos_Control(1, 1, speed, speed_add, location, 0, 0);
		delay_ms(10);
		Emm_V5_Pos_Control(2, 1, speed, speed_add, location, 0, 0);
		delay_ms(10);
		Emm_V5_Pos_Control(3, 0, speed, speed_add, location, 0, 0);
		delay_ms(10);
		Emm_V5_Pos_Control(4, 0, speed, speed_add, location, 0, 0);
//		delay_ms(10);
//		Emm_V5_Synchronous_motion(0x00);
		while(1)
		{
			if (rxCmd[1] == 0xFD && rxCmd[2] == 0x9F && rxCmd [3] == 0x6B)
			{
				delay_ms(20);
				break;
		}
			Gyro_DisplayPoll(); /* Keep OLED live while waiting. */
		}
	}
	
	if (dir == 4)
	{
		delay_ms(10);
		Emm_V5_Pos_Control(1, 0, speed, speed_add, location, 0, 0);
		delay_ms(10);
		Emm_V5_Pos_Control(2, 0, speed, speed_add, location, 0, 0);
		delay_ms(10);
		Emm_V5_Pos_Control(3, 1, speed, speed_add, location, 0, 0);
		delay_ms(10);
		Emm_V5_Pos_Control(4, 1, speed, speed_add, location, 0, 0);
//		delay_ms(10);
//		Emm_V5_Synchronous_motion(0x00);
		while(1)
		{
			if (rxCmd[1] == 0xFD && rxCmd[2] == 0x9F && rxCmd [3] == 0x6B)
			{	
				delay_ms(20);
				break;
		}
			Gyro_DisplayPoll(); /* Keep OLED live while waiting. */
		}
	}
	
	if (dir == 5)
	{
		delay_ms(10);
		Emm_V5_Pos_Control(1, 1, speed, speed_add, location, 0, 0);
		delay_ms(10);
		Emm_V5_Pos_Control(2, 1, speed, speed_add, location, 0, 0);
		delay_ms(10);
		Emm_V5_Pos_Control(3, 1, speed, speed_add, location, 0, 0);
		delay_ms(10);
		Emm_V5_Pos_Control(4, 1, speed, speed_add, location, 0, 0);
//		delay_ms(10);
//		Emm_V5_Synchronous_motion(0x00);
		while(1)
		{
			if (rxCmd[1] == 0xFD && rxCmd[2] == 0x9F && rxCmd [3] == 0x6B)
			{	
				delay_ms(20);
				break;
		}
			Gyro_DisplayPoll(); /* Keep OLED live while waiting. */
		}
	}
	
	if (dir == 6)
	{
		delay_ms(10);
		Emm_V5_Pos_Control(1, 0, speed, speed_add, location, 0, 0);
		delay_ms(10);
		Emm_V5_Pos_Control(2, 0, speed, speed_add, location, 0, 0);
		delay_ms(10);
		Emm_V5_Pos_Control(3, 0, speed, speed_add, location, 0, 0);
		delay_ms(10);
		Emm_V5_Pos_Control(4, 0, speed, speed_add, location, 0, 0);
		
				uint32_t timeout = 0;   // 计时器，单位ms
//		delay_ms(10);
//		Emm_V5_Synchronous_motion(0x00);
		while(1)
		{
			
											// 每次循环延时一点，避免CPU卡死
        delay_ms(10);
        timeout += 10;

        // 超时处理
        if (timeout >= 3000)
        {
            // 这里可以做强制停止或报错处理
            Emm_V5_Stop_Now(1, 0);
            Emm_V5_Stop_Now(2, 0);
            Emm_V5_Stop_Now(3, 0);
            Emm_V5_Stop_Now(4, 0);
            break;
        }
			
			if (rxCmd[1] == 0xFD && rxCmd[2] == 0x9F && rxCmd [3] == 0x6B)
			{	
				delay_ms(20);
				break;
		}
			Gyro_DisplayPoll(); /* Keep OLED live while waiting. */
		}
	}
	
	if (dir == 7)
	{
		delay_ms(10);
		Emm_V5_Pos_Control(2, 1, speed, speed_add, location, 0, 0);
		delay_ms(10);
		Emm_V5_Pos_Control(3, 0, speed, speed_add, location, 0, 0);
//		delay_ms(10);
//		Emm_V5_Synchronous_motion(0x00);
		while(1)
		{
			if (rxCmd[1] == 0xFD && rxCmd[2] == 0x9F && rxCmd [3] == 0x6B)
			{	
				delay_ms(20);
				break;
		}
			Gyro_DisplayPoll(); /* Keep OLED live while waiting. */
		}
	}
	
	if (dir == 8)
	{
		delay_ms(10);
		Emm_V5_Pos_Control(2, 0, speed, speed_add, location, 0, 0);
		delay_ms(10);
		Emm_V5_Pos_Control(3, 1, speed, speed_add, location, 0, 0);
//		delay_ms(10);
//		Emm_V5_Synchronous_motion(0x00);
		while(1)
		{
			if (rxCmd[1] == 0xFD && rxCmd[2] == 0x9F && rxCmd [3] == 0x6B)
			{	
				delay_ms(20);
				break;
		}
			Gyro_DisplayPoll(); /* Keep OLED live while waiting. */
		}
	}
	
	
	
	if (dir == 9) /* Wang Kai diagonal: motor 1 dir=1, motor 4 dir=0. */
	{
		delay_ms(10);
		Emm_V5_Pos_Control(1, 1, speed, speed_add, location, 0, 0);
		delay_ms(10);
		Emm_V5_Pos_Control(4, 0, speed, speed_add, location, 0, 0);
//		delay_ms(10);
//		Emm_V5_Synchronous_motion(0x00);
		while(1)
		{
			if (rxCmd[1] == 0xFD && rxCmd[2] == 0x9F && rxCmd [3] == 0x6B)
			{	
				delay_ms(20);
				break;
		}
			Gyro_DisplayPoll(); /* Keep OLED live while waiting. */
		}
	}

    if (dir == 10)
    {
        /* Fixed dir6-compatible 90-degree arc; location remains unused. */
        Car_Arc90(speed);
    }
			if (dir == 11)  //粗加工到暂存区和暂存到转盘  转弯
	{

		Emm_V5_Pos_Control(1, 1, 500, 150, location, 0, 0);
		delay_ms(10);
		Emm_V5_Pos_Control(2, 0, 500, 150, (location+8700*4), 0, 0);
		delay_ms(10);
		Emm_V5_Pos_Control(3, 1, 500, 150, location, 0, 0);
		delay_ms(10);
		Emm_V5_Pos_Control(4, 0, 500, 150, (location+8700*4), 0, 0);
		while(1)
		{
			if (rxCmd[1] == 0xFD && rxCmd[2] == 0x9F && rxCmd [3] == 0x6B)
			{	
				delay_ms(20);
				break;
		}
			Gyro_DisplayPoll(); /* Keep OLED live while waiting. */
		}
	}
    car_debug_active = 0;
}
