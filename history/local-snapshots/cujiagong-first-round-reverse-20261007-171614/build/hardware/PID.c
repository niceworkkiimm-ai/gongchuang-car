#include "stm32f10x.h"                  // Device header
#include <stdio.h> // 确保包含标准库（如需要）

#include <limits.h> // 用于限制PWM范围

#define TURN_TARGET 1 // 根据实际需求设置
#define PWM_MIN -180//角度环限幅
#define PWM_MAX 180




float Err=0,last_err=0,next_err=0,pwm=0,add=0;

float p=0.1,v=0,d=0;//速度环PID



int16_t myabs(int a)
{ 		   
	  int temp;
		if(a<0)  temp=a;  
	  else temp=a;
	  return temp;
}

//void pwm_control()
//{
//    if(pwm>360)
//        pwm=360;
//    if(pwm<-360)
//        pwm=-360;
//}




float pid3(int16_t speed1,float tar1)//a
{
//	float p=3.9,i=0.01,d=0;
    speed1=myabs(speed1);
    Err=tar1-speed1;
    add=p*(Err-last_err)+v*(Err)+d*(Err+next_err-2*last_err);
    pwm+=add;
//    pwm_control();
    next_err=last_err;
    last_err=Err;
    return pwm;
}





float pid2(int16_t speed1,float tar1)//b
{
//	float p=3.9,i=0.01,d=0;
    speed1=myabs(speed1);
    Err=tar1-speed1;
    add=p*(Err-last_err)+v*(Err)+d*(Err+next_err-2*last_err);
    pwm+=add;
//    pwm_control();
    next_err=last_err;
    last_err=Err;
    return pwm;
}





int angle(float Angle,float Gyroy,float Mechanical_Angle)
{
	float Kp = 1; //       
  float Kd = 0.1;//      
	float Bias; //角度误差值
	int balance_up; //直立环控制PWM
	Bias=Angle-Mechanical_Angle; //角度误差值==测量的俯仰角-理想角度（机械平衡角度）
	balance_up= Kp*Bias+ Kd*Gyroy; //计算平衡控制的电机PWM  PD控制   Up_balance_KP是P系数,Up_balance_KD是D系数
	return balance_up;
} 
