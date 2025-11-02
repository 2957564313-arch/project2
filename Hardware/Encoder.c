#include "stm32f10x.h"

static int32_t position1 = 0, position2 = 0;
static int16_t last_count1 = 0, last_count2 = 0;

void Encoder_Init(void)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4 | RCC_APB1Periph_TIM3, ENABLE);
    
    // 配置编码器1引脚 (PA6, PA7)
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    // 配置编码器2引脚 (PB6, PB7)
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    
    // 配置编码器1 (TIM3)
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    TIM_TimeBaseStructure.TIM_Prescaler = 0;
    TIM_TimeBaseStructure.TIM_Period = 0xFFFF;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseStructure.TIM_ClockDivision = 0;
    TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);
    
    TIM_EncoderInterfaceConfig(TIM3, TIM_EncoderMode_TI12, TIM_ICPolarity_Rising, TIM_ICPolarity_Rising);
    TIM_SetCounter(TIM3, 0);
    TIM_Cmd(TIM3, ENABLE);
    
    // 配置编码器2 (TIM4)
    TIM_TimeBaseInit(TIM4, &TIM_TimeBaseStructure);
    
    TIM_EncoderInterfaceConfig(TIM4, TIM_EncoderMode_TI12, TIM_ICPolarity_Rising, TIM_ICPolarity_Rising);
    TIM_SetCounter(TIM4, 0);
    TIM_Cmd(TIM4, ENABLE);
    
    // 初始化上次计数
    last_count1 = TIM_GetCounter(TIM3);
    last_count2 = TIM_GetCounter(TIM4);
}

// 获取编码器速度（1ms内的脉冲数）
int16_t Encoder_Get_Speed(uint8_t num)
{
    int16_t current_count = 0;
    int16_t delta = 0;
    
    if(num == 1)
    {
        current_count = TIM_GetCounter(TIM3);
        delta = current_count - last_count1;
        last_count1 = current_count;
        
        // 处理溢出
        if (delta > 32768) delta -= 65536;
        if (delta < -32768) delta += 65536;
        
        position1 += delta;
    }
    else if(num == 2)
    {
        current_count = TIM_GetCounter(TIM4);
        delta = current_count - last_count2;
        last_count2 = current_count;
        
        // 处理溢出
        if (delta > 32768) delta -= 65536;
        if (delta < -32768) delta += 65536;
        
        position2 += delta;
    }
    
    return (int16_t)delta*1.85;
}

// 获取编码器累计位置
int32_t Encoder_Get_Position(uint8_t num)
{
    if(num == 1)
        return position1;
    else
        return position2;
}

// 清零累计位置
void Encoder_Clear_TotalCount(uint8_t num)
{
    if(num == 1)
    {
        position1 = 0;
        last_count1 = TIM_GetCounter(TIM3);
    }
    else
    {
        position2 = 0;
        last_count2 = TIM_GetCounter(TIM4);
    }
}
