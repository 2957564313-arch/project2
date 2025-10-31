#include "stm32f10x.h"

// 速度PID参数 - 增量式PID
static float speed_kp = 40.0f, speed_ki = 5.0f, speed_kd = 1.0f;
static float speed_error[3] = {0, 0, 0};
static float speed_output = 0;

// 位置PID参数 - 位置式PID，调整参数减少振动
static float pos_kp = 0.3f, pos_ki = 0.0f, pos_kd = 0.1f;
static float pos_error = 0, pos_last_error = 0;
static float pos_integral = 0;

// 速度增量式PID计算
int16_t Speed_PID_Calculate(int16_t target, int16_t actual)
{
    speed_error[2] = speed_error[1];
    speed_error[1] = speed_error[0];
    speed_error[0] = target - actual;
    
    float delta_output = speed_kp * (speed_error[0] - speed_error[1]) + 
                        speed_ki * speed_error[0] + 
                        speed_kd * (speed_error[0] - 2*speed_error[1] + speed_error[2]);
    
    speed_output += delta_output;
    
    // 输出限幅
    if(speed_output > 1000) speed_output = 1000;
    if(speed_output < -1000) speed_output = -1000;
    
    return (int16_t)speed_output;
}

// 位置位置式PID计算
// 位置PID计算 - 增加输出滤波
// 位置PID计算 - 使用纯比例控制 + 强滤波
int16_t Position_PID_Calculate(int32_t target, int32_t actual)
{
    int32_t error = target - actual;
    
    // 方法1：纯比例控制，大幅降低增益
    float output = pos_kp * error;
    
    // 方法2：或者使用非线性控制
    // float output = pos_kp * error;
    // if(abs(error) < 50) {
    //     output *= 0.3f;  // 小误差时降低增益
    // }
    
    // 强输出滤波
    static float filtered_output = 0;
    filtered_output = 0.1f * output + 0.9f * filtered_output;
    
    // 极低的输出限幅
    if(filtered_output > 50) filtered_output = 50;
    if(filtered_output < -50) filtered_output = -50;
    
    return (int16_t)filtered_output;
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
    pos_ki = i;
    pos_kd = d;
}

// 重置PID状态
void Speed_PID_Reset(void)
{
    speed_error[0] = speed_error[1] = speed_error[2] = 0;
    speed_output = 0;
}

void Position_PID_Reset(void)
{
    pos_error = 0;
    pos_last_error = 0;
    pos_integral = 0;
}
