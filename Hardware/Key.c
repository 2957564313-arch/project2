#include "stm32f10x.h"
#include "Delay.h"

uint32_t GetSystemTick(void);

void Key_Init(void)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;    
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
}

uint8_t Key_GetNum(void)
{
    static uint8_t last_state = 1; // 默认上拉，未按下为1
    static uint32_t last_press_time = 0;
    uint8_t current_state = GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0);
    uint32_t current_time = GetSystemTick(); 
    
    // 检测按键按下（从1变为0）
    if(last_state == 1 && current_state == 0)
    {
        // 简单消抖，检查是否持续按下
        Delay_ms(50); // 短延时检查
		
        if(GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0) == 0)
        {
            last_state = 0;
            last_press_time = current_time;
            return 1;
        }
    }
    
    // 检测按键释放（从0变为1）
    if(last_state == 0 && current_state == 1)
    {
        last_state = 1;
    }
    
    return 0;
}

volatile uint32_t system_tick = 0;



uint32_t GetSystemTick(void)
{
    return system_tick;
}
