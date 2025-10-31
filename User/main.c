#include "stm32f10x.h"
#include "OLED.h"
#include "Serial.h"
#include "PID.h"
#include "Key.h"
#include "Timer.h"
#include "Encoder.h"
#include "Motor.h"

uint8_t current_mode = 2; // 1:速度控制模式, 2:位置跟随模式
int16_t target_speed = 0;

int main(void)
{
    // 初始化所有外设
    Key_Init();
    OLED_Init();
    Serial_Init();
    Timer_Init();
    Encoder_Init();
    PWM_Init();
    
    // 设置PID参数 - 优化位置PID参数减少振动
    Speed_PID_SetParams(5.0f, 2.0f, 1.0f);
    Position_PID_SetParams(0.15199f, 0.0f, 0.50f); // 减小位置PID参数
    
    // 初始化电机停止
    GPIO_ResetBits(GPIOB, GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15);
    TIM_SetCompare3(TIM2, 0);
    TIM_SetCompare4(TIM2, 0);
    
    // OLED显示初始化
    OLED_ShowString(1, 1, "Mode:");
    OLED_ShowNum(1, 6, current_mode, 1);
    OLED_ShowString(2, 1, "Speed Control");
    
    while(1)
    {
        // 检测按键切换模式
        uint8_t key_num = Key_GetNum();
        if(key_num != 0)
        {
            current_mode = (current_mode == 1) ? 2 : 1;
            OLED_ShowNum(1, 6, current_mode, 1);
            
            if(current_mode == 1)
            {
                OLED_ShowString(2, 1, "Speed Control");
                // 停止两个电机
                GPIO_ResetBits(GPIOB, GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15);
                TIM_SetCompare3(TIM2, 0);
                TIM_SetCompare4(TIM2, 0);
                Speed_PID_Reset();
            }
            else
            {
                OLED_ShowString(2, 1, "Pos Following");
                // 重置编码器位置
                Encoder_Clear_TotalCount(1);
                Encoder_Clear_TotalCount(2);
                Position_PID_Reset();
                target_position2 = 0;
                last_position1 = Encoder_Get_Position(1);
            }
        }
    }
}
