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

volatile uint8_t car13_phase = 0;
volatile uint8_t car13_error = 0;

/* Only dir13 needs a bounded stop: its four wheels use speed mode before exit.
 * 1=参数/无陀螺仪帧，2=陀螺仪断帧，3=前段/转弯超时，4=后段超时，5=电机错误，6=读位置失败。
 */
static void Car13_Stop(uint8_t error)
{
    uint8_t addr;
    car13_error = error;
    car13_phase = 0xFF;
    WheelReply_End();
    for (addr = 1; addr <= 4; ++addr)
    {
        Emm_V5_Stop_Now(addr, 0);
        delay_ms(10);
    }
    for (;;)
    {
        Gyro_DisplayPoll();
        delay_ms(10);
    }
}

/* One addressed 36 reply; Emm position is signed magnitude, 65536 counts/rev.
 * Use the existing complete-frame FIFO; no new UART interrupt/status parser.
 */
static int64_t Car13_ReadPosition(void)
{
    uint8_t attempt, found;
    uint16_t i;
    uint32_t start, mask, magnitude;
    int64_t position = 0;
    for (attempt = 0; attempt < 3; ++attempt)
    {
        mask = __get_PRIMASK();
        __disable_irq();
        rxFrameFlag = false;
        __set_PRIMASK(mask);
        Emm_V5_Read_Sys_Params(1, S_CPOS);
        start = CameraDebug_NowMs();
        while ((uint32_t)(CameraDebug_NowMs() - start) < 100UL)
        {
            found = 0;
            mask = __get_PRIMASK();
            __disable_irq();
            if (rxFrameFlag)
            {
                rxFrameFlag = false;
                for (i = 0; i + 8U <= rxCount; ++i)
                {
                    if (rxCmd[i] == 1 && rxCmd[i+1] == 0x36 &&
                        rxCmd[i+2] <= 1 && rxCmd[i+7] == 0x6B)
                    {
                        magnitude = ((uint32_t)rxCmd[i+3] << 24) |
                                    ((uint32_t)rxCmd[i+4] << 16) |
                                    ((uint32_t)rxCmd[i+5] << 8) | rxCmd[i+6];
                        position = rxCmd[i+2] ? -(int64_t)magnitude : (int64_t)magnitude;
                        found = 1;
                        break;
                    }
                }
            }
            __set_PRIMASK(mask);
            if (found) return position;
            Gyro_DisplayPoll();
            delay_ms(1);
        }
    }
    Car13_Stop(6);
    return 0;
}

static void Car13_Speeds(uint16_t inner, uint16_t outer, uint8_t acc)
{
    Emm_V5_Vel_Control(1, 1, inner, acc, 0);
    delay_ms(10);
    Emm_V5_Vel_Control(3, 1, inner, acc, 0);
    delay_ms(10);
    Emm_V5_Vel_Control(2, 0, outer, acc, 0);
    delay_ms(10);
    Emm_V5_Vel_Control(4, 0, outer, acc, 0);
}

static void Car13_CheckGyro(uint32_t *frames, uint32_t *last, uint32_t now)
{
    if (*frames != gyro_yaw_frame_count)
    {
        *frames = gyro_yaw_frame_count;
        *last = now;
    }
    if ((uint32_t)(now - *last) >= CAR_DIR13_GYRO_TIMEOUT_MS) Car13_Stop(2);
}

void car(dir,speed,location)
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
				//delay_ms(20);
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

			if (dir == 10)   //转盘到粗加工转弯
	{
		Emm_V5_Pos_Control(1, 1, 500, 150, 2225*4, 0, 0);
		delay_ms(10);
		Emm_V5_Pos_Control(2, 0, 500, 150, 9000*4, 0, 0);
		delay_ms(10);
		Emm_V5_Pos_Control(3, 1, 500, 150, 2225*4, 0, 0);
		delay_ms(10);
		Emm_V5_Pos_Control(4, 0, 500, 150, 9000*4, 0, 0);
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
			if (dir == 11)  //粗加工到暂存区和暂存到转盘  转弯
	{

		Emm_V5_Pos_Control(1, 0, 500, 150, 2225*4, 0, 0);
		delay_ms(10);
		Emm_V5_Pos_Control(2, 1, 500, 150, 9000*4, 0, 0);
		delay_ms(10);
		Emm_V5_Pos_Control(3, 0, 500, 150, 2225*4, 0, 0);
		delay_ms(10);
		Emm_V5_Pos_Control(4, 1, 500, 150, 9000*4, 0, 0);
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
    if (dir == 12) /* 直行+原dir10转弯合为一条位置指令，中间不等到位。 */
    {
        /* location只填直行脉冲；两侧再分别加当前dir10的转弯脉冲。
         * 1/3轮附加2225*4，2/4轮附加9000*4；方向沿用dir2和dir10。
         * 同速执行，不是恒定半径圆弧；保留原发送间隔和到位判断。
         */
        Emm_V5_Pos_Control(1, 1, speed, 150, location + 2225*4, 0, 0);
        delay_ms(10);
        Emm_V5_Pos_Control(2, 0, speed, 150, location + 9000*4, 0, 0);
        delay_ms(10);
        Emm_V5_Pos_Control(3, 1, speed, 150, location + 2225*4, 0, 0);
        delay_ms(10);
        Emm_V5_Pos_Control(4, 0, speed, 150, location + 9000*4, 0, 0);
        while (1)
        {
            if (rxCmd[1] == 0xFD && rxCmd[2] == 0x9F && rxCmd[3] == 0x6B)
            {
                delay_ms(20);
                break;
            }
            Gyro_DisplayPoll();
        }
    }
    if (dir == 13) /* 四轮统一控制：实测前直行->比例速度转弯->统一后直行。 */
    {
        uint8_t addr;
        uint16_t inner_speed;
        uint32_t start, now, last_gyro, frames, timeout_ms;
        int64_t origin, travel;
        float previous, current, delta, turned = 0.0f;
        car13_error = 0;
        car13_phase = 1;
        if (speed <= 0 || speed > 500 || location < 0 ||
            CAR_DIR13_AFTER_PULSES <= 0 || !CAR_DIR13_PULSES_PER_REV ||
            CAR_DIR13_INNER_PULSES <= 0 || CAR_DIR13_OUTER_PULSES <= CAR_DIR13_INNER_PULSES ||
            CAR_DIR13_TURN_DEG <= 0.0f || CAR_DIR13_TURN_DEG >= 180.0f || !gyro_yaw_frame_count)
            Car13_Stop(1);
        timeout_ms = CAR_DIR13_MOVE_TIMEOUT_MS * 500UL / (uint32_t)speed;
        frames = gyro_yaw_frame_count;
        last_gyro = CameraDebug_NowMs();
        origin = Car13_ReadPosition();
        Car13_Speeds((uint16_t)speed, (uint16_t)speed, 150);
        start = CameraDebug_NowMs();

        /* Third argument is the measured front straight, not front+inner arc pulses. */
        while (1)
        {
            travel = Car13_ReadPosition() - origin;
            if (travel < 0) travel = -travel;
            now = CameraDebug_NowMs();
            Car13_CheckGyro(&frames, &last_gyro, now);
            if ((uint32_t)(now - start) >= timeout_ms) Car13_Stop(3);
            if ((uint64_t)travel * CAR_DIR13_PULSES_PER_REV >= (uint64_t)location * 65536UL) break;
            Gyro_DisplayPoll();
            delay_ms(5);
        }

        /* All four remain velocity-controlled. Inner/outer target speeds keep
         * the current 2225:9000 ratio; no single wheel stops on its own target.
         */
        car13_phase = 2;
        previous = anglea;
        inner_speed = (uint16_t)((uint32_t)speed * CAR_DIR13_INNER_PULSES / CAR_DIR13_OUTER_PULSES);
        if (!inner_speed) inner_speed = 1;
        Car13_Speeds(inner_speed, (uint16_t)speed, CAR_DIR13_TURN_ACC);
        start = CameraDebug_NowMs();
        while (1)
        {
            Gyro_DisplayPoll();
            now = CameraDebug_NowMs();
            Car13_CheckGyro(&frames, &last_gyro, now);
            if ((uint32_t)(now - start) >= timeout_ms) Car13_Stop(3);
            current = anglea;
            delta = current - previous;
            if (delta > 180.0f) delta -= 360.0f;
            else if (delta < -180.0f) delta += 360.0f;
            turned += delta;
            previous = current;
            if (turned <= -CAR_DIR13_TURN_DEG) break;
            delay_ms(5);
        }

        /* Preserve the user's stop+100ms at the turn exit, with normal TX gaps. */
        for (addr = 1; addr <= 4; ++addr)
        {
            Emm_V5_Stop_Now(addr, 0);
            if (addr < 4) delay_ms(10);
        }
        delay_ms(CAR_DIR13_EXIT_SETTLE_MS);

        /* All four switch to the same position mode after stopping. Restore the
         * normal acc=150 instead of acc=0's sudden full-speed restart.
         */
        car13_phase = 3;
        WheelReply_Begin();
        for (addr = 1; addr <= 4; ++addr)
        {
            WheelReply_Arm(addr);
            Emm_V5_Pos_Control(addr, (addr == 1 || addr == 3) ? 1 : 0,
                               speed, 150, CAR_DIR13_AFTER_PULSES, 0, 0);
            if (addr < 4) delay_ms(10);
        }
        start = CameraDebug_NowMs();
        while ((wheel_reply_done_mask & 0x0F) != 0x0F)
        {
            if (wheel_reply_error_code) Car13_Stop(5);
            if ((uint32_t)(CameraDebug_NowMs() - start) >= timeout_ms) Car13_Stop(4);
            Gyro_DisplayPoll();
            delay_ms(1);
        }
        if (wheel_reply_error_code) Car13_Stop(5);
        WheelReply_End();
        car13_phase = 4;
    }
    car_debug_active = 0;
}
