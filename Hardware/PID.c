#include "stm32f10x.h"
#include "stdlib.h"

// 速度PID参数
static float speed_Out = 0;		
static float speed_kp = 2.0f, speed_ki = 0.5f, speed_kd = 0.1f;
static float speed_error[3] = {0, 0, 0};		// 误差队列：当前、上次、上上次误差

// 位置PID参数 - 使用纯比例控制以减少振动
static float pos_kp = 0.15f;  // 位置环比例系数，值很小以减少超调

/**
 * @brief 速度增量式PID计算
 * @param target 目标速度值
 * @param actual 实际速度值（编码器反馈）
 * @return PID计算后的PWM输出值
 * 
 * 增量式PID公式：
 * Δu = Kp*(e(k)-e(k-1)) + Ki*e(k) + Kd*(e(k)-2e(k-1)+e(k-2))
 * 其中e(k)为当前误差，e(k-1)为上次误差，e(k-2)为上上次误差
 */
int16_t Speed_PID_Calculate(int16_t target, int16_t actual)
{
	// 更新误差队列：将旧的误差向后移动
    speed_error[2] = speed_error[1];
    speed_error[1] = speed_error[0];
    speed_error[0] = target - actual;		// 计算当前误差
    
	// 增量式PID计算
    speed_Out += speed_kp * (speed_error[0] - speed_error[1]) + 
                 speed_ki * speed_error[0] + 
                 speed_kd * (speed_error[0] - 2 * speed_error[1] + speed_error[2]);
    
    // 输出限幅：防止PID输出过大损坏电机
    if(speed_Out > 800) speed_Out = 800;
    if(speed_Out < -800) speed_Out = -800;
    
    return (int16_t)speed_Out;
}

/**
 * @brief 位置PID计算 - 简化为纯比例控制
 * @param target 目标位置
 * @param actual 实际位置
 * @return 位置控制输出值
 * 
 * 位置环使用纯比例控制，避免积分项引起的振荡和超调
 */
int16_t Position_PID_Calculate(int32_t target, int32_t actual)
{
    int32_t error = target - actual;		// 位置误差
    
    // 纯比例控制，增益非常低
    float output = pos_kp * error;
    
    // 严格的输出限幅：位置控制不需要大功率输出
    if(output > 50) output = 50;
    if(output < -50) output = -50;
    
    return (int16_t)output;
}

/**
 * @brief 设置速度PID参数
 * @param p 比例系数
 * @param i 积分系数
 * @param d 微分系数
 */
void Speed_PID_SetParams(float p, float i, float d)
{
    speed_kp = p;
    speed_ki = i;
    speed_kd = d;
}

/**
 * @brief 设置位置PID参数
 * @param p 比例系数
 * @param i 积分系数（实际未使用）
 * @param d 微分系数（实际未使用）
 * 
 * 位置环只使用比例控制，积分和微分参数被忽略
 */
void Position_PID_SetParams(float p, float i, float d)
{
    pos_kp = p;
    // 忽略积分和微分，使用纯比例
}

/**
 * @brief 重置速度PID状态
 * 
 * 在模式切换或目标速度改变时调用，防止积分饱和
 */
void Speed_PID_Reset(void)
{
    speed_error[0] = speed_error[1] = speed_error[2] = 0;		// 清空误差队列
    speed_Out = 0;	// 清空误差队列
}
