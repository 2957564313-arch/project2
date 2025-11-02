#include "stm32f10x.h"
#include "Encoder.h"
#include "Serial.h"
#include "PID.h"
#include "Motor.h"
#include <stdlib.h>

extern uint8_t current_mode;
extern int16_t target_speed;

/**
 * @brief 定时器2初始化 - 10ms中断
 * 
 * 定时器配置为10ms周期，用于：
 * 1. 读取编码器速度
 * 2. 执行PID计算
 * 3. 更新电机PWM输出
 * 4. 发送数据到上位机
 */
void Timer_Init(void)
{
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
    
    TIM_InternalClockConfig(TIM2);
    
    TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
    TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInitStructure.TIM_Period = 1000 - 1;     // 10ms中断
    TIM_TimeBaseInitStructure.TIM_Prescaler = 720 - 1;
    TIM_TimeBaseInitStructure.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit(TIM2, &TIM_TimeBaseInitStructure);
    
    TIM_ClearFlag(TIM2, TIM_FLAG_Update);
    TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);
    
    // 设置正确的NVIC优先级分组
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    
    // 配置Timer中断优先级（较低优先级，避免阻塞其他中断）
    NVIC_InitTypeDef NVIC_InitStructure;
    NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;  // 降低抢占优先级
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_Init(&NVIC_InitStructure);
    
    TIM_Cmd(TIM2, ENABLE);
}

/**
 * @brief 定时器2中断服务函数 - 每10ms执行一次
 * 
 * 这是系统的核心控制循环，负责：
 * 1. 读取传感器数据
 * 2. 执行控制算法
 * 3. 更新执行器输出
 * 4. 数据通信
 */
void TIM2_IRQHandler(void)
{
    if(TIM_GetITStatus(TIM2, TIM_IT_Update) == SET)
    {
        // 快速读取编码器数据 - 获取当前状态
        int16_t speed1 = Encoder_Get_Speed(1);
        int16_t speed2 = Encoder_Get_Speed(2);
        int32_t position1 = Encoder_Get_Position(1);
        int32_t position2 = Encoder_Get_Position(2);
        
        // 模式1：速度控制模式 - 控制电机1的速度
        if(current_mode == 1)
{
    
    int16_t adjusted_target = target_speed;		 // 调整后的目标速度
    
    // 目标速度补偿：轻微提高目标值来克服系统静摩擦
    if(abs(target_speed) > 5) {
        if(target_speed > 0) {
            adjusted_target = target_speed + 2;  // +2的补偿
        } else {
            adjusted_target = target_speed - 2;
        }
    }
    // 执行速度PID计算，获取PWM输出
    int16_t pwm1 = Speed_PID_Calculate(adjusted_target, speed1);
    Motor_Set_Speed(1, pwm1); // 设置电机1速度
}
// 模式2：位置跟随模式 - 电机2跟随电机1的位置
        else
        {
            // 位置跟随模式
            static uint8_t pulse_active = 0;	// 脉冲激活标志
            static uint8_t pulse_counter = 0;	// 脉冲计数器
            
            int32_t position_error = position1 - position2;		// 位置误差
            
			// 如果误差较大且未激活脉冲，开始校正
            if(!pulse_active && abs(position_error) > 100)
            {
                // 根据误差方向设置校正脉冲
                int16_t correction_pwm = (position_error > 0) ? 120 : -120;
                Motor_Set_Speed(2, correction_pwm);	// 给电机2一个脉冲
                pulse_active = 1;	// 标记脉冲激活
                pulse_counter = 0;	// 重置计数器
            }
			// 脉冲激活中，计数等待
            else if(pulse_active)
            {
                pulse_counter++;
                if(pulse_counter > 3) // 30ms后停止
                {
                    Motor_Set_Speed(2, 0);
                    pulse_active = 0;
                }
            }
            else		// 无误差或误差很小，保持停止
            {
                Motor_Set_Speed(2, 0);
            }
            
            last_position1 = position1;	 // 更新上次位置
            Motor_Set_Speed(1, 0);		// 位置模式下电机1自由转动
        }
        
        // 发送数据到上位机
        static uint8_t send_counter = 0;
        if(send_counter++ >= 2) // 每20ms发送一次，减轻负担
        {
            if(USART_GetFlagStatus(USART1, USART_FLAG_TC)) // 检查串口是否空闲
            {
                USART_Send_Data(speed1,target_speed);	// 发送速度数据
            }
            send_counter = 0;	// 重置计数器
        }
        
        TIM_ClearITPendingBit(TIM2, TIM_IT_Update);	 // 清除中断标志
    }
}
