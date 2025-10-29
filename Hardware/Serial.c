#include "stm32f10x.h"                  // Device header
#include <stdio.h>
#include <string.h>
#include "OLED.h"
#include <stdlib.h>

int16_t target_speed = 0;

void Serial_Init(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1,ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);
	
	//配置TX引脚
	GPIO_InitTypeDef GPIO_InitStruture;
	GPIO_InitStruture.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStruture.GPIO_Pin =GPIO_Pin_9;
	GPIO_InitStruture.GPIO_Speed =GPIO_Speed_50MHz ;
	GPIO_Init(GPIOA,&GPIO_InitStruture);
	
	//配置RX引脚
	GPIO_InitStruture.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_InitStruture.GPIO_Pin =GPIO_Pin_10;
	GPIO_Init(GPIOA,&GPIO_InitStruture);
	
	//配置串口
	USART_InitTypeDef USART_InitStruture;
	USART_InitStruture.USART_BaudRate =115200 ;
	USART_InitStruture.USART_HardwareFlowControl =USART_HardwareFlowControl_None ;
	USART_InitStruture.USART_Mode =USART_Mode_Tx|USART_Mode_Rx ;
	USART_InitStruture.USART_Parity = USART_Parity_No;
	USART_InitStruture.USART_StopBits = USART_StopBits_1;
	USART_InitStruture.USART_WordLength = USART_WordLength_8b;
	USART_Init(USART1,&USART_InitStruture);
	
	//使能接收中断
	USART_ITConfig(USART1,USART_IT_RXNE,ENABLE);
	
	//配置NVIC
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	
	NVIC_InitTypeDef NVIC_InitStruture;
	NVIC_InitStruture.NVIC_IRQChannel =USART1_IRQn;
	NVIC_InitStruture.NVIC_IRQChannelCmd =ENABLE;
	NVIC_InitStruture.NVIC_IRQChannelPreemptionPriority =1;
	NVIC_InitStruture.NVIC_IRQChannelSubPriority =1;
	NVIC_Init(&NVIC_InitStruture);
	
	USART_Cmd(USART1,ENABLE);
	
}

void USART1_IRQHandler(void)
{
    if(USART_GetITStatus(USART1, USART_IT_RXNE))
    {
        static char cmd_buffer[32];
        static uint8_t cmd_index = 0;
        static uint8_t cmd_started = 0;
        
        char received_char = USART_ReceiveData(USART1);
        
        if(received_char == '@') 
        {
            cmd_index = 0;
            cmd_started = 1;
            cmd_buffer[cmd_index++] = received_char;
        }
        else if(cmd_started) 
        {
            if(received_char == '\r' || received_char == '\n') 
            {
                cmd_buffer[cmd_index] = '\0';
               
                // 解析速度命令
                char *speed_cmd = strstr(cmd_buffer, "@speed%");
                if(speed_cmd != NULL) 
                {
                    char *number_str = speed_cmd + 7; // 跳过 "@speed%"
                    target_speed = atoi(number_str);
                }
                
                cmd_started = 0;
                cmd_index = 0;
            }
            else if(cmd_index < 31) 
            {
                cmd_buffer[cmd_index++] = received_char;
            }
            else 
            {
                // 缓冲区溢出，重置
                cmd_started = 0;
                cmd_index = 0;
            }
        }
        
        USART_ClearITPendingBit(USART1, USART_IT_RXNE);
    }
}

//重定向printf，改变输出的目的地
int fputc(int ch,FILE *f)		
{
	USART_SendData(USART1,(uint8_t)ch);
	
	while(USART_GetFlagStatus(USART1,USART_FLAG_TC) == RESET);
	return ch;
}


void USART_Send_Data(int16_t speed1,int16_t speed2,int32_t pos1,int32_t pos2)
{
	printf("%.2f,%.2f,%.2f,%.2f\n",(float)speed1,(float)speed2,(float)pos1,(float)pos2);
}

