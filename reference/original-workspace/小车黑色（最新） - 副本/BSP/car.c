#include "car.h"
#include "delay.h"
#include "Emm_V5.h"
#include "usart.h"

volatile float anglea = 0;

static const uint8_t speed_add = 150;

void car(int dir, int speed, int location)
{
	while ((rxCmd[1] != 0x00) || (rxCmd[2] != 0x00) || (rxCmd[3] != 0x00))
	{

		rxCmd[1] = 0x00;
		rxCmd[2] = 0x00;
		rxCmd[3] = 0x00;
	}

	if (dir == 0)
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

		while(1)
		{
			if (rxCmd[1] == 0xFD && rxCmd[2] == 0x9F && rxCmd [3] == 0x6B)
			{	
				delay_ms(20);
				break;
		}
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

		while(1)
		{
			if (rxCmd[1] == 0xFD && rxCmd[2] == 0x9F && rxCmd [3] == 0x6B)
			{	
				delay_ms(20);
				break;
		}
		}
		
	}

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

		uint32_t timeout = 0;   // 计时器，单位ms
				
		while(1)
		{

        delay_ms(10);
        timeout += 10;

        if (timeout >= 3000)
        {

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

		while(1)
		{

        delay_ms(10);
        timeout += 10;

        if (timeout >= 3000)
        {

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

		while(1)
		{
			if (rxCmd[1] == 0xFD && rxCmd[2] == 0x9F && rxCmd [3] == 0x6B)
			{
				delay_ms(20);
				break;
		}
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

		while(1)
		{
			if (rxCmd[1] == 0xFD && rxCmd[2] == 0x9F && rxCmd [3] == 0x6B)
			{	
				delay_ms(20);
				break;
		}
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

		while(1)
		{
			if (rxCmd[1] == 0xFD && rxCmd[2] == 0x9F && rxCmd [3] == 0x6B)
			{	
				delay_ms(20);
				break;
		}
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

		while(1)
		{

        delay_ms(10);
        timeout += 10;

        if (timeout >= 3000)
        {

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
	
	if (dir == 7)
	{
		delay_ms(10);
		Emm_V5_Pos_Control(2, 1, speed, speed_add, location, 0, 0);
		delay_ms(10);
		Emm_V5_Pos_Control(3, 0, speed, speed_add, location, 0, 0);

		while(1)
		{
			if (rxCmd[1] == 0xFD && rxCmd[2] == 0x9F && rxCmd [3] == 0x6B)
			{	
				delay_ms(20);
				break;
		}
		}
	}
	
	if (dir == 8)
	{
		delay_ms(10);
		Emm_V5_Pos_Control(2, 0, speed, speed_add, location, 0, 0);
		delay_ms(10);
		Emm_V5_Pos_Control(3, 1, speed, speed_add, location, 0, 0);

		while(1)
		{
			if (rxCmd[1] == 0xFD && rxCmd[2] == 0x9F && rxCmd [3] == 0x6B)
			{	
				delay_ms(20);
				break;
		}
		}
	}

			if (dir == 10)   //转盘到粗加工转弯
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
		}
	}
}
