#include "stm32f10x.h"                  // Device header
#include "OLED.h"
#include "Serial.h"
#include "PID.h"
#include "Key.h"
#include "Timer.h"
#include "Encoder.h"
#include "Motor.h"

uint8_t current_mode = 1;		//模式1为速度控制模式，模式2为位置跟随模式
extern int16_t target_speed;

int main()
{
	Key_Init();
	OLED_Init();
	Serial_Init();
	Timer_Init();
	Encoder_Init();
	PWM_Init();
	
	Speed_PID_SetParams(2.0f,0.5f,0.1f);
	Position_PID_SetParams(0.5f,0.01f,0.3f);
	
	//初始化电机停止
	GPIO_ResetBits(GPIOB,GPIO_Pin_12|GPIO_Pin_13 |GPIO_Pin_14 |GPIO_Pin_15 );
	TIM_SetCompare3(TIM2,0);		
	TIM_SetCompare4(TIM2,0);		
	
	OLED_ShowString(1,1,"Mode:");
	OLED_ShowNum(1,6,current_mode,1);
	OLED_ShowString(2,1,"Speed Control");
	
	target_position2 = Encoder_Get_Position(2);
	last_position1 = Encoder_Get_Position(1);
	
	while(1)
	{
		uint8_t key_num = Key_GetNum();
		if(key_num != 0)
		{
			current_mode = (current_mode == 1) ? 2 : 1;
			OLED_ShowNum(1,6,current_mode,1);
			
			if(current_mode == 1)
			{
				OLED_ShowString(2,1,"Speed Control");

				//停止电机2
				GPIO_ResetBits(GPIOB,GPIO_Pin_14 | GPIO_Pin_15);
				TIM_SetCompare4(TIM2,0);
			}
			else
			{
				OLED_ShowString(2,1,"Pos Following");
				
				Encoder_Clear_TotalCount(1);		//切换模式时重置编码器位置
				Encoder_Clear_TotalCount(2);
			}
		}
		
	}
}
