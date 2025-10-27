#include "stm32f10x.h"                  // Device header
#include "Encoder.h"
#include "Serial.h"
#include "PID.h"
#include "PWM.h"

extern float Actual;
int32_t motor1_position = 0;
int32_t motor2_target = 0;

void Timer_Init(void)
{
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2,ENABLE);
	
	TIM_InternalClockConfig(TIM2);
	
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
	TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInitStructure.TIM_Period =1000-1 ;		//10ms中断
	TIM_TimeBaseInitStructure.TIM_Prescaler = 720-1;
	TIM_TimeBaseInitStructure.TIM_RepetitionCounter =0 ;
	TIM_TimeBaseInit(TIM2,&TIM_TimeBaseInitStructure);
	
	TIM_ClearFlag(TIM2,TIM_FLAG_Update);
	TIM_ITConfig(TIM2,TIM_IT_Update,ENABLE);
	
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	
	NVIC_InitTypeDef NVIC_InitStruture;
	NVIC_InitStruture.NVIC_IRQChannel = TIM2_IRQn;
	NVIC_InitStruture.NVIC_IRQChannelCmd = ENABLE;
	NVIC_InitStruture.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStruture.NVIC_IRQChannelSubPriority =1;
	NVIC_Init(&NVIC_InitStruture);
	
	TIM_Cmd(TIM2,ENABLE);
	
}
void TIM2_IRQHandler(void)
{
	if(TIM_GetITStatus(TIM2,TIM_IT_Update)== SET)
	{
		Motor_Process();
		TIM_ClearITPendingBit(TIM2,TIM_IT_Update);
	}
}
