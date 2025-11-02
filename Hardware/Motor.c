#include "stm32f10x.h"

int32_t last_position1=0;
int32_t target_position2=0;
extern uint8_t current_mode;
void PWM_Init(void)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
    
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2 | GPIO_Pin_3;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    
    TIM_InternalClockConfig(TIM2);
    
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseStructure.TIM_Period = 1000 - 1;
    TIM_TimeBaseStructure.TIM_Prescaler = 72 - 1;
    TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);
    
    TIM_OCInitTypeDef TIM_OCInitStructure;
    TIM_OCStructInit(&TIM_OCInitStructure);
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_Pulse = 0;
    
    TIM_OC3Init(TIM2, &TIM_OCInitStructure);
    TIM_OC4Init(TIM2, &TIM_OCInitStructure);
    
    TIM_Cmd(TIM2, ENABLE);
}

void Motor_Set_Speed(uint8_t motor_num, int16_t speed)
{
    
    // 限幅
    if(speed > 1000) speed = 1000;
    if(speed < -1000) speed = -1000;
    
    // 针对位置模式的特殊保护
    if(current_mode == 2) {
        // 在位置模式下，对两个电机都使用严格限制
        if(speed > 200) speed = 200;
        if(speed < -200) speed = -200;
        
        // 提高死区
        if(speed > 0 && speed < 120) speed = 0;
        else if(speed < 0 && speed > -120) speed = 0;
    } else {
        // 速度模式的正常死区
        if(speed > 0 && speed < 30) speed = 0;
        else if(speed < 0 && speed > -30) speed = 0;
    }
    
    if(motor_num == 1)
    {
        if(speed == 0) {
            // 完全停止模式
            GPIO_ResetBits(GPIOB, GPIO_Pin_12 | GPIO_Pin_13);
        } else if(speed > 0) {
            GPIO_SetBits(GPIOB, GPIO_Pin_12);
            GPIO_ResetBits(GPIOB, GPIO_Pin_13);
        } else {
            GPIO_ResetBits(GPIOB, GPIO_Pin_12);
            GPIO_SetBits(GPIOB, GPIO_Pin_13);
            speed = -speed;
        }
        TIM_SetCompare3(TIM2, speed);
    }
    else if(motor_num == 2)
    {
        if(speed == 0) {
            // 完全停止模式
            GPIO_ResetBits(GPIOB, GPIO_Pin_14 | GPIO_Pin_15);
        } else if(speed > 0) {
            GPIO_SetBits(GPIOB, GPIO_Pin_14);
            GPIO_ResetBits(GPIOB, GPIO_Pin_15);
        } else {
            GPIO_ResetBits(GPIOB, GPIO_Pin_14);
            GPIO_SetBits(GPIOB, GPIO_Pin_15);
            speed = -speed;
        }
        TIM_SetCompare4(TIM2, speed);
    }
}
