#include "stm32f10x.h"                  // Device header
#include "Encoder.h"
#include "PID.h"
#include "Serial.h"

int32_t target_position2 = 0;
int32_t last_position1 = 0;
extern uint8_t current_mode;
extern int16_t target_speed;

void PWM_Init(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO,ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2,ENABLE);
	
	//配置PWM引脚
	GPIO_InitTypeDef GPIO_InitStruture;
	GPIO_InitStruture.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStruture.GPIO_Pin = GPIO_Pin_2|GPIO_Pin_3;		
	GPIO_InitStruture.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA,&GPIO_InitStruture);
	
	//配置方向控制引脚
	GPIO_InitStruture.GPIO_Pin = GPIO_Pin_12 |GPIO_Pin_13|GPIO_Pin_14|GPIO_Pin_15;
	GPIO_InitStruture.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStruture.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB,&GPIO_InitStruture);
	
	TIM_InternalClockConfig(TIM2);
	
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStruture;
	TIM_TimeBaseStruture.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseStruture.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseStruture.TIM_Period = 1000-1;		
	TIM_TimeBaseStruture.TIM_Prescaler = 72-1;   
	TIM_TimeBaseStruture.TIM_RepetitionCounter = 0;
	TIM_TimeBaseInit(TIM2,&TIM_TimeBaseStruture);
	
	//配置PWM通道
	TIM_OCInitTypeDef TIM_OCInitStruture;
	TIM_OCStructInit(&TIM_OCInitStruture);
	TIM_OCInitStruture.TIM_OCMode = TIM_OCMode_PWM1;
	TIM_OCInitStruture.TIM_OCPolarity = TIM_OCPolarity_High;
	TIM_OCInitStruture.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStruture.TIM_Pulse = 0;  
	TIM_OC3Init(TIM2,&TIM_OCInitStruture);		//电机1
	TIM_OC4Init(TIM2,&TIM_OCInitStruture);		//电机2
	TIM_CtrlPWMOutputs(TIM2,ENABLE);
	
	TIM_Cmd(TIM2,ENABLE);
	
	//初始化后停止所有电机
	GPIO_ResetBits(GPIOB,GPIO_Pin_12|GPIO_Pin_13 |GPIO_Pin_14 |GPIO_Pin_15 );
	TIM_SetCompare3(TIM2,0);		//电机1停止
	TIM_SetCompare4(TIM2,0);		//电机2停止
	
}

void Motor_Set_Speed(uint8_t motor_num,int16_t speed)
{
	if(speed > 800) speed = 800;
	if(speed < -800) speed = -800 ;
	
	if(motor_num == 1)
	{
		if(speed >= 0)
	{
		//正转
		GPIO_SetBits(GPIOB,GPIO_Pin_12);
		GPIO_ResetBits(GPIOB,GPIO_Pin_13);		
	}
	else
	{
		//反转
		GPIO_SetBits(GPIOB,GPIO_Pin_13);
		GPIO_ResetBits(GPIOB,GPIO_Pin_12);
		speed = -speed;
	}
	TIM_SetCompare3(TIM2,speed);		//将速度传入，改变占空比
	}
	else if(motor_num == 2)
	{
		if(speed >= 0)
	{
		//正转
		GPIO_SetBits(GPIOB,GPIO_Pin_14);
		GPIO_ResetBits(GPIOB,GPIO_Pin_15);
			
	}
	else
	{
		//反转
		GPIO_SetBits(GPIOB,GPIO_Pin_15);
		GPIO_ResetBits(GPIOB,GPIO_Pin_14);
		speed = -speed;
	}
	TIM_SetCompare4(TIM2,speed);
	}	
}
void Motor_Process(void)		//电机控制处理，在定时器中断调用
{
	
	int16_t speed1 = Encoder_Get_Speed(1);
	int16_t speed2 = Encoder_Get_Speed(2);
	
	int32_t position1 = Encoder_Get_Position(1);
	int32_t position2 = Encoder_Get_Position(2);
	
	if(current_mode == 1)
	{
		int16_t pwm1 = Speed_PID_Calculate(target_speed,speed1);
		Motor_Set_Speed(1,pwm1);
		
		//确保电机2停止
		GPIO_ResetBits(GPIOB,GPIO_Pin_14 | GPIO_Pin_15);
		TIM_SetCompare4(TIM2,0);		
	}
	else
	{
		int32_t position1_change = position1 - last_position1;
		if(position1_change != 0)		
		{
			target_position2 += position1_change;
		}
		
		int16_t pwm2 =Position_PID_Calculate(target_position2,position2);
		
		
		
		if(pwm2 > 50) pwm2 = 50;		//限制最大输出
		if(pwm2 < -50) pwm2 = -50;
		
		if(abs(target_position2 - position2) < 10)		//小误差停止
		{
			pwm2 = 0;
		}
		
		Motor_Set_Speed(2,pwm2);
		Motor_Set_Speed(1,0);	//电机1手动转动
		
		last_position1 = position1;
		
	}
	USART_Send_Data(speed1,speed2,position1,position2);
	
}

