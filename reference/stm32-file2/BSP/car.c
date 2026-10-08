#include "delay.h"
#include "Emm_V5.h"
#include "usart.h"

float anglea=0;

int speed_add=150;

void car(dir,speed,location)
{
	while ((rxCmd[1] != 0x00) || (rxCmd[2] != 0x00) || (rxCmd[3] != 0x00))
	{

		rxCmd[1] = 0x00;
		rxCmd[2] = 0x00;
		rxCmd[3] = 0x00;
	}

	if (dir == 0)   //原地旋转调角度（闭环速度模式，四轮同向旋转到目标角度）
	{

			if ((anglea < speed ) || ((anglea > speed) && (anglea >135)))
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
			if ((anglea > speed) || ((anglea < speed) && (anglea <-135)))
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
			if (anglea == speed || anglea == 179 || anglea == -179)
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
		}
	}
		
	if (dir == 1) //向后走
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
		}
	}
	
	if (dir == 2)   //向前走
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
		}
		
	}
	
	
	
		//*********粗加工物料专用********
	
		if (dir == 21)    //向后走（带3秒超时，粗加工专用）
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
		}
	}
	
	if (dir == 22)   //向前走（带3秒超时，粗加工专用）
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
		}
		
	}
	
	
	
	
	if (dir == 3)   //右平移（左前右前正转，左后右后反转）
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
		}
	}
	
	if (dir == 4)   //左平移（左前右前反转，左后右后正转）
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
		}
	}
	
	if (dir == 5)   //原地右转/顺时针（四轮全正转）
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
		}
	}
	
	if (dir == 6)   //原地左转/逆时针（四轮全反转，带3秒超时）
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
		}
	}
	
	if (dir == 7)//向右后方
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
		}
	}
	
	if (dir == 8)   //向左前方（与dir=7相反，仅控制M2反转M3正转）
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
		}
	}
	
	if (dir == 9)   //左后方（M1正转M4反转）
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
		}
	}
	
	if (dir == 10)   //右前方（M1反转M4正转，与dir=12相反）
	{
		delay_ms(10);
		Emm_V5_Pos_Control(1, 0, speed, speed_add, location, 0, 0);
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
		}
	}
	
	

			if (dir == 11)   //转盘到粗加工转弯（固定）
	{
		Emm_V5_Pos_Control(1, 1, 500, 150, 2225*4, 0, 0);
		delay_ms(10);
		Emm_V5_Pos_Control(2, 0, 500, 150, 10925*4, 0, 0);
		delay_ms(10);
		Emm_V5_Pos_Control(3, 1, 500, 150, 2225*4, 0, 0);
		delay_ms(10);
		Emm_V5_Pos_Control(4, 0, 500, 150, 10925*4, 0, 0);
		while(1)
		{
			if (rxCmd[1] == 0xFD && rxCmd[2] == 0x9F && rxCmd [3] == 0x6B)
			{	
				delay_ms(20);
				break;
		}
		}
	}
			if (dir == 12)  //粗加工到暂存区和暂存到转盘  转弯
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
		}
	}
			if (dir == 13)  //12的全返 转弯
	{

		Emm_V5_Pos_Control(1, 0, 500, 150, (location+8700*4), 0, 0);
		delay_ms(10);
		Emm_V5_Pos_Control(2, 1, 500, 150, location, 0, 0);
		delay_ms(10);
		Emm_V5_Pos_Control(3, 0, 500, 150, (location+8700*4), 0, 0);
		delay_ms(10);
		Emm_V5_Pos_Control(4, 1, 500, 150, location, 0, 0);
		while(1)
		{
			if (rxCmd[1] == 0xFD && rxCmd[2] == 0x9F && rxCmd [3] == 0x6B)
			{	
				delay_ms(20);
				break;
		}
		}
	}
}
