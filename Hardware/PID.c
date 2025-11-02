#include "stm32f10x.h"
#include "stdlib.h"

// 速度PID参数
static float speed_Out = 0;		
static float speed_kp = 2.0f, speed_ki = 0.5f, speed_kd = 0.1f;
static float speed_error[3] = {0, 0, 0};

// 位置PID参数 - 使用纯比例控制，参数大幅降低
static float pos_kp = 0.15f;  // 进一步降低比例系数

// 速度增量式PID计算
int16_t Speed_PID_Calculate(int16_t target, int16_t actual)
{
    speed_error[2] = speed_error[1];
    speed_error[1] = speed_error[0];
    speed_error[0] = target - actual;
    
    speed_Out += speed_kp * (speed_error[0] - speed_error[1]) + 
                 speed_ki * speed_error[0] + 
                 speed_kd * (speed_error[0] - 2 * speed_error[1] + speed_error[2]);
    
    // 输出限幅
    if(speed_Out > 800) speed_Out = 800;
    if(speed_Out < -800) speed_Out = -800;
    
    return (int16_t)speed_Out;
}

// 位置PID计算 - 简化为纯比例控制
int16_t Position_PID_Calculate(int32_t target, int32_t actual)
{
    int32_t error = target - actual;
    
    // 纯比例控制，增益非常低
    float output = pos_kp * error;
    
    // 输出限幅 - 非常严格
    if(output > 50) output = 50;
    if(output < -50) output = -50;
    
    return (int16_t)output;
}

// PID参数设置
void Speed_PID_SetParams(float p, float i, float d)
{
    speed_kp = p;
    speed_ki = i;
    speed_kd = d;
}

void Position_PID_SetParams(float p, float i, float d)
{
    pos_kp = p;
    // 忽略积分和微分，使用纯比例
}

// 修复重置函数
void Speed_PID_Reset(void)
{
    speed_error[0] = speed_error[1] = speed_error[2] = 0;
    speed_Out = 0;
}

void Position_PID_Reset(void)
{
    // 位置PID无需重置内部状态
}
