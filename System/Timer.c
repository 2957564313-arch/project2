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
    
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    
    NVIC_InitTypeDef NVIC_InitStructure;
    NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_Init(&NVIC_InitStructure);
    
    TIM_Cmd(TIM2, ENABLE);
}

void TIM2_IRQHandler(void)
{
    if(TIM_GetITStatus(TIM2, TIM_IT_Update) == SET)
    {
        int16_t speed1 = Encoder_Get_Speed(1);
        int16_t speed2 = Encoder_Get_Speed(2);
        int32_t position1 = Encoder_Get_Position(1);
        int32_t position2 = Encoder_Get_Position(2);
        
        if(current_mode == 1)
        {
            // 速度控制模式 - 保持不变
            int16_t pwm1 = Speed_PID_Calculate(target_speed, speed1);
            Motor_Set_Speed(1, pwm1);
            
            // 确保电机2完全停止
            GPIO_ResetBits(GPIOB, GPIO_Pin_14 | GPIO_Pin_15);
            TIM_SetCompare4(TIM2, 0);
        }
        else
        {
            // ==================== 新的位置跟随策略 ====================
            
            // 方法1：直接位置跟随（非增量式）
            target_position2 = position1;  // 电机2直接跟随电机1的绝对位置
            
            int16_t pwm2 = Position_PID_Calculate(target_position2, position2);
            
            // 多重防振措施：
            
            // 1. 死区控制 - 增大死区范围
            int32_t position_error = target_position2 - position2;
            if(abs(position_error) < 15) {  // 增大死区
                pwm2 = 0;
            }
            
            // 2. 输出限幅 - 大幅降低最大输出
            if(pwm2 > 80) pwm2 = 80;
            if(pwm2 < -80) pwm2 = -80;
            
            // 3. 输出平滑 - 强滤波
            static int16_t last_pwm2 = 0;
            int16_t pwm_diff = pwm2 - last_pwm2;
            if(abs(pwm_diff) > 10) {  // 限制变化率
                pwm2 = last_pwm2 + (pwm_diff > 0 ? 10 : -10);
            }
            last_pwm2 = pwm2;
            
            Motor_Set_Speed(2, pwm2);
            
            // 电机1完全自由
            Motor_Set_Speed(1, 0);
            
            // 调试输出
            static uint16_t debug_counter = 0;
            if(debug_counter++ >= 20) { // 每200ms输出一次
               
                debug_counter = 0;
            }
            // ==================== 新的位置跟随策略结束 ====================
        }
        
        // 发送数据到上位机
        USART_Send_Data(speed1, speed2, position1, position2);
        
        TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
    }
}