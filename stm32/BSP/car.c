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

/* Only dir13 needs a bounded stop: its outer wheels use continuous speed mode.
 * 1=参数/无陀螺仪帧，2=陀螺仪断帧，3=转弯超时，4=后直行超时，5=电机返回错误。
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
    if (dir == 13) /* 前直行->同向转弯->后直行；转弯出口不停车调正。 */
    {
        uint8_t addr;
        uint32_t start, now, last_gyro, frames, timeout_ms;
        float previous, current, delta, turned = 0.0f;
        car13_error = 0;
        car13_phase = 1;
        if (speed <= 0 || speed > 500 || location < 0 ||
            CAR_DIR13_AFTER_PULSES <= 0 || CAR_DIR13_TURN_DEG <= 0.0f ||
            CAR_DIR13_TURN_DEG >= 180.0f || !gyro_yaw_frame_count)
            Car13_Stop(1);
        timeout_ms = CAR_DIR13_MOVE_TIMEOUT_MS * 500UL / (uint32_t)speed;
        frames = gyro_yaw_frame_count;
        previous = anglea;
        start = last_gyro = CameraDebug_NowMs();

        /* Inner wheels use the existing dir12 distance. Outer wheels keep running,
         * so their pulse target cannot expire before the turn reaches its heading.
         */
        Emm_V5_Pos_Control(1, 1, speed, 150, location + CAR_DIR13_INNER_PULSES, 0, 0);
        delay_ms(10);
        Emm_V5_Vel_Control(2, 0, speed, 150, 0);
        delay_ms(10);
        Emm_V5_Pos_Control(3, 1, speed, 150, location + CAR_DIR13_INNER_PULSES, 0, 0);
        delay_ms(10);
        Emm_V5_Vel_Control(4, 0, speed, 150, 0);

        /* Accumulate signed yaw across +/-180. No car(0), no stop at the exit. */
        while (1)
        {
            Gyro_DisplayPoll();
            now = CameraDebug_NowMs();
            if (frames != gyro_yaw_frame_count)
            {
                frames = gyro_yaw_frame_count;
                last_gyro = now;
            }
            if ((uint32_t)(now - last_gyro) >= CAR_DIR13_GYRO_TIMEOUT_MS)
                Car13_Stop(2);
            if ((uint32_t)(now - start) >= timeout_ms)
                Car13_Stop(3);
            current = anglea;
            delta = current - previous;
            if (delta > 180.0f) delta -= 360.0f;
            else if (delta < -180.0f) delta += 360.0f;
            turned += delta;
            previous = current;
            if (turned <= -CAR_DIR13_TURN_DEG) break;
            delay_ms(5);
        }

        /* Same-direction position motion follows immediately, at the same RPM.
         * acc=0 selects direct-speed motion: no deliberate ramp back from zero.
         * Start a fresh existing FD tracker for the final segment; no status query.
         */
        car13_phase = 2;
        WheelReply_Begin();
        for (addr = 1; addr <= 4; ++addr)
        {
            WheelReply_Arm(addr);
            Emm_V5_Pos_Control(addr, (addr == 1 || addr == 3) ? 1 : 0,
                               speed, 0, CAR_DIR13_AFTER_PULSES, 0, 0);
            if (addr < 4) delay_ms(10);
        }
        start = CameraDebug_NowMs();
        while ((wheel_reply_done_mask & 0x0F) != 0x0F)
        {
            if (wheel_reply_error_code) Car13_Stop(5);
            if ((uint32_t)(CameraDebug_NowMs() - start) >= timeout_ms)
                Car13_Stop(4);
            Gyro_DisplayPoll();
            delay_ms(1);
        }
        if (wheel_reply_error_code) Car13_Stop(5);
        WheelReply_End();
        car13_phase = 3;
    }
    car_debug_active = 0;
}
