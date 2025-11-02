#include "stm32f10x.h"
#include "Encoder.h"
#include "Serial.h"
#include "PID.h"
#include "Motor.h"
#include <stdlib.h>

extern uint8_t current_mode;
extern int16_t target_speed;

static int32_t target_position2 = 0;
static int32_t last_position1 = 0;

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

void TIM2_IRQHandler(void)
{
    if(TIM_GetITStatus(TIM2, TIM_IT_Update) == SET)
    {
        // 快速读取编码器数据
        int16_t speed1 = Encoder_Get_Speed(1);
        int16_t speed2 = Encoder_Get_Speed(2);
        int32_t position1 = Encoder_Get_Position(1);
        int32_t position2 = Encoder_Get_Position(2);
        
        
        if(current_mode == 1)
{
    // 速度控制模式
    int16_t adjusted_target = target_speed;
    
    // 根据目标速度给予轻微补偿
    if(abs(target_speed) > 5) {
        // 轻微提高目标值来克服系统损失
        if(target_speed > 0) {
            adjusted_target = target_speed + 2;  // +2的补偿
        } else {
            adjusted_target = target_speed - 2;
        }
    }
    
    int16_t pwm1 = Speed_PID_Calculate(adjusted_target, speed1);
    Motor_Set_Speed(1, pwm1);
}
        else
        {
            // 位置跟随模式 - 简化处理
            static int32_t last_position1 = 0;
            static uint8_t pulse_active = 0;
            static uint8_t pulse_counter = 0;
            
            int32_t position_error = position1 - position2;
            
            if(!pulse_active && abs(position_error) > 100)
            {
                // 开始脉冲
                int16_t correction_pwm = (position_error > 0) ? 120 : -120;
                Motor_Set_Speed(2, correction_pwm);
                pulse_active = 1;
                pulse_counter = 0;
            }
            else if(pulse_active)
            {
                pulse_counter++;
                if(pulse_counter > 3) // 30ms后停止
                {
                    Motor_Set_Speed(2, 0);
                    pulse_active = 0;
                }
            }
            else
            {
                Motor_Set_Speed(2, 0);
            }
            
            last_position1 = position1;
            Motor_Set_Speed(1, 0);
        }
        
        // 发送数据到上位机（如果串口不忙）
        static uint8_t send_counter = 0;
        if(send_counter++ >= 2) // 每20ms发送一次，减轻负担
        {
            if(USART_GetFlagStatus(USART1, USART_FLAG_TC)) // 检查串口是否空闲
            {
                USART_Send_Data(speed1,target_speed);
            }
            send_counter = 0;
        }
        
        TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
    }
}