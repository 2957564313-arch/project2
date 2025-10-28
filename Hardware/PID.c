#include "stm32f10x.h"                  // Device header

//速度PID参数
static float speed_Out = 0;		
static float speed_kp=2.0f,speed_ki=0.5f,speed_kd=0.1f;
static float speed_error[3] = {0,0,0};

//位置PID参数
static float pos_Out = 0;		
static float pos_kp=0.5f,pos_ki=0.01f,pos_kd=0.3f;
static float pos_error[3] = {0,0,0};

//速度增量式PID计算
int16_t Speed_PID_Calculate(int16_t target,int16_t actual)
{
	speed_error[2] = speed_error[1];
	speed_error[1] = speed_error[0];
	speed_error[0] = target - actual;
	
	speed_Out += speed_kp*(speed_error[0] - speed_error[1]) + speed_ki *speed_error[0] + speed_kd * (speed_error[0] - 2*speed_error[1] +speed_error[2]);
	
	//输出限幅
	if(speed_Out > 800 ) speed_Out = 800;
	if(speed_Out < -800 ) speed_Out = -800;
	
	return (int16_t)speed_Out;
}

//位置增量式PID计算
int16_t Position_PID_Calculate(int32_t target,int32_t actual)
{
	pos_error[2] = pos_error[1];
	pos_error[1] = pos_error[0];
	pos_error[0] = target - actual;
	
	pos_Out += pos_kp*(pos_error[0] - pos_error[1]) + pos_ki *pos_error[0] + pos_kd * (pos_error[0] - 2*pos_error[1] +pos_error[2]);
	
	//输出限幅
	if(pos_Out > 500 ) pos_Out = 500;
	if(pos_Out < -500 ) pos_Out = -500;
	
	return (int16_t)pos_Out;
}

//PID参数设置
void Speed_PID_SetParams(float p,float i, float d)
{
	speed_kp = p;
	speed_ki = i;
	speed_kd = d;
}

void Position_PID_SetParams(float p,float i, float d)
{
	pos_kp = p;
	pos_ki = i;
	pos_kd = d;
}
