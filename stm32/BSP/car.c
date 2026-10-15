#include "gyro_log.h"
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

/* car(0) only: keep the user's angular tolerance. */
#define CAR_HEADING_TOLERANCE_DEG 0.4f
#define CAR_HEADING_FINE_ZONE_DEG 1.5f
#define CAR_HEADING_FAST_RPM 13U /* Project 10 velocity; acceleration remains 10. */
#define CAR_HEADING_FINE_RPM 2U
#define CAR_HEADING_COMMAND_GAP_MS 10U
#define CAR_HEADING_BRAKE_SECONDS 0.07f
#define CAR_HEADING_MAX_BRAKE_DEG 1.0f
#define CAR_HEADING_SETTLE_MIN_MS 80UL
#define CAR_HEADING_STABLE_MS 120UL
#define CAR_HEADING_STABLE_RANGE_DEG 0.12f
#define CAR_HEADING_STABLE_RATE_DPS 1.5f
#define CAR_HEADING_STALE_MS 100UL
#define CAR_HEADING_TIMEOUT_MS 15000UL

extern volatile float angular_velocity_z;
volatile uint8_t car_heading_status = CAR_HEADING_IDLE;
volatile float car_heading_error_deg = 0.0f;
volatile float car_heading_rate_dps = 0.0f;

static float Car_Abs(float value)
{
    return value < 0.0f ? -value : value;
}

static float Car_HeadingError(float target, float yaw)
{
    float error = target - yaw;
    if (error > 180.0f) error -= 360.0f;
    else if (error < -180.0f) error += 360.0f;
    return error;
}

static void Car_ReadHeading(float *yaw, float *rate, uint32_t *frames)
{
    uint32_t mask = __get_PRIMASK();
    __disable_irq();
    *yaw = global_angle; /* Latest validated UART3 angle, not the 20ms copy. */
    *rate = angular_velocity_z;
    *frames = gyro_yaw_frame_count;
    __set_PRIMASK(mask);
}

static void Car_StopHeadingWheels(void)
{
    uint8_t motor;
    /* Stop motor 1 immediately; keep existing bus spacing between motors. */
    for (motor = 1; motor <= 4; ++motor)
    {
        if (motor != 1) delay_ms(CAR_HEADING_COMMAND_GAP_MS);
        Emm_V5_Stop_Now(motor, 0);
        GyroLog_Mark(GLOG_STOP_SENT, motor, 0);
    }
}

static void Car_SendHeadingVelocity(uint8_t direction, uint16_t rpm)
{
    uint8_t motor;
    for (motor = 1; motor <= 4; ++motor)
    {
        delay_ms(CAR_HEADING_COMMAND_GAP_MS);
        Emm_V5_Vel_Control(motor, direction, rpm, 10, 0);
        GyroLog_Mark(GLOG_VELOCITY_SENT, motor, direction);
    }
}

static uint8_t Car_HeadingFailure(uint8_t status)
{
    Car_StopHeadingWheels();
    car_heading_status = status;
    return 0;
}

static uint8_t Car_AlignHeading(float target)
{
    float yaw, rate, error, distance, anchor = 0.0f, brake;
    uint32_t frames, last_frames, now, start, last_frame_ms, stopped_ms;
    uint32_t stable_start_ms = 0;
    uint16_t commanded_rpm = 0, requested_rpm;
    uint8_t moving = 0, direction = 0, requested_direction, new_frame;
    uint8_t stable_started = 0, stable_frames = 0, window_logged = 0;
    uint8_t stable_in_window = 0;
    uint8_t initial_start = 1;

    car_heading_status = CAR_HEADING_RUNNING;
    Car_ReadHeading(&yaw, &rate, &last_frames);
    start = CameraDebug_NowMs();
    last_frame_ms = start;
    stopped_ms = start;

    while (1)
    {
        Car_ReadHeading(&yaw, &rate, &frames);
        now = CameraDebug_NowMs();
        new_frame = frames != last_frames;
        if (new_frame)
        {
            last_frames = frames;
            last_frame_ms = now;
        }
        /* Reject NaN/out-of-range data without allowing the next route step. */
        if (!(yaw >= -180.0f && yaw <= 180.0f &&
              rate >= -2000.0f && rate <= 2000.0f &&
              target >= -180.0f && target <= 180.0f))
            return Car_HeadingFailure(CAR_HEADING_BAD_DATA);
        error = Car_HeadingError(target, yaw);
        distance = Car_Abs(error);
        car_heading_error_deg = error;
        car_heading_rate_dps = rate;
        if ((uint32_t)(now - start) >= CAR_HEADING_TIMEOUT_MS)
            return Car_HeadingFailure(CAR_HEADING_TIMEOUT);
        if ((uint32_t)(now - last_frame_ms) >= CAR_HEADING_STALE_MS)
            return Car_HeadingFailure(CAR_HEADING_STALE);

        if (distance <= CAR_HEADING_TOLERANCE_DEG && !window_logged)
        {
            GyroLog_Mark(GLOG_HEADING_WINDOW, 0, 0);
            window_logged = 1;
        }
        requested_direction = error >= 0.0f ? 1U : 0U;
        if (initial_start)
        {
            /* Start on a fresh angle without the final-settling wait. */
            if (new_frame)
            {
                initial_start = 0;
                if (distance > CAR_HEADING_TOLERANCE_DEG)
                {
                    requested_rpm = distance > CAR_HEADING_FINE_ZONE_DEG ?
                        CAR_HEADING_FAST_RPM : CAR_HEADING_FINE_RPM;
                    Car_SendHeadingVelocity(requested_direction, requested_rpm);
                    direction = requested_direction;
                    commanded_rpm = requested_rpm;
                    moving = 1;
                }
                else
                {
                    Car_StopHeadingWheels();
                    stopped_ms = CameraDebug_NowMs();
                }
            }
        }
        else if (moving)
        {
            brake = CAR_HEADING_TOLERANCE_DEG;
            if (commanded_rpm == CAR_HEADING_FINE_RPM &&
                ((direction && rate > 0.0f) || (!direction && rate < 0.0f)))
            {
                brake = Car_Abs(rate) * CAR_HEADING_BRAKE_SECONDS;
                if (brake < CAR_HEADING_TOLERANCE_DEG)
                    brake = CAR_HEADING_TOLERANCE_DEG;
                if (brake > CAR_HEADING_MAX_BRAKE_DEG)
                    brake = CAR_HEADING_MAX_BRAKE_DEG;
            }
            /* Also stop on a crossing; never reverse four moving wheels. */
            if (distance <= brake || requested_direction != direction)
            {
                Car_StopHeadingWheels();
                stopped_ms = CameraDebug_NowMs();
                moving = 0;
                stable_started = 0;
                stable_frames = 0;
            }
            else
            {
                requested_rpm = distance > CAR_HEADING_FINE_ZONE_DEG ?
                    CAR_HEADING_FAST_RPM : CAR_HEADING_FINE_RPM;
                /* Only reduce an active turn; resume speed after a full stop. */
                if (requested_rpm < commanded_rpm)
                {
                    Car_SendHeadingVelocity(direction, requested_rpm);
                    commanded_rpm = requested_rpm;
                }
            }
        }
        else if (new_frame)
        {
            /* Verify actual settling using BOTH rate and a fresh-angle range. */
            if ((uint32_t)(now - stopped_ms) < CAR_HEADING_SETTLE_MIN_MS ||
                Car_Abs(rate) > CAR_HEADING_STABLE_RATE_DPS)
            {
                stable_started = 0;
                stable_frames = 0;
            }
            else
            {
                if (!stable_started ||
                    Car_Abs(Car_HeadingError(anchor, yaw)) >
                    CAR_HEADING_STABLE_RANGE_DEG)
                {
                    anchor = yaw;
                    stable_start_ms = now;
                    stable_started = 1;
                    stable_frames = 1;
                    stable_in_window = distance <= CAR_HEADING_TOLERANCE_DEG;
                }
                else
                {
                    if (stable_frames < 255U) ++stable_frames;
                    if (distance > CAR_HEADING_TOLERANCE_DEG)
                        stable_in_window = 0;
                }
                if (stable_frames >= 3U &&
                    (uint32_t)(now - stable_start_ms) >= CAR_HEADING_STABLE_MS)
                {
                    if (stable_in_window && distance <= CAR_HEADING_TOLERANCE_DEG)
                    {
                        car_heading_status = CAR_HEADING_SUCCESS;
                        return 1;
                    }
                    requested_rpm = distance > CAR_HEADING_FINE_ZONE_DEG ?
                        CAR_HEADING_FAST_RPM : CAR_HEADING_FINE_RPM;
                    Car_SendHeadingVelocity(requested_direction, requested_rpm);
                    direction = requested_direction;
                    commanded_rpm = requested_rpm;
                    moving = 1;
                    stable_started = 0;
                    stable_frames = 0;
                    window_logged = 0;
                }
            }
        }
        Gyro_DisplayPoll();
        delay_ms(5);
    }
}

void car(int dir, int speed, int location)
{
    car_debug_mode = (int16_t)dir;
    car_debug_target = dir == 0 ? (int16_t)speed : 0;
    car_debug_active = 1;
    GyroLog_CarBegin(dir, speed);
	while ((rxCmd[1] != 0x00) || (rxCmd[2] != 0x00) || (rxCmd[3] != 0x00))
	{

		rxCmd[1] = 0x00;
		rxCmd[2] = 0x00;
		rxCmd[3] = 0x00;
	}

	if (dir == 0)
	{
        if (!Car_AlignHeading((float)speed))
        {
            /* Fault hold: do not run the next route step with a bad heading. */
            while (1)
            {
                Gyro_DisplayPoll();
                delay_ms(20);
            }
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
	
	
	
	if (dir == 13) /* 直行+原dir10转弯合为一条位置指令，中间不等到位。 */
    {
        /* location只填直行脉冲；两侧再分别加当前dir10的转弯脉冲。
         * 1/3轮附加2225*4，2/4轮附加9000*4；方向沿用dir2和dir10。
         * 同速执行，不是恒定半径圆弧；保留原发送间隔和到位判断。
         */
        Emm_V5_Pos_Control(1, 0, speed, 150, location + 2225*4, 0, 0);
        delay_ms(10);
        Emm_V5_Pos_Control(2, 1, speed, 150, location + 9000*4, 0, 0);
        delay_ms(10);
        Emm_V5_Pos_Control(3, 0, speed, 150, location + 2225*4, 0, 0);
        delay_ms(10);
        Emm_V5_Pos_Control(4, 1, speed, 150, location + 9000*4, 0, 0);
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
    car_debug_active = 0;
    GyroLog_CarEnd();
}
