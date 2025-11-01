#include "stm32f10x.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "PID.h"

extern int16_t target_speed;

void Serial_Init(void)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    
    // 配置TX引脚
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    // 配置RX引脚
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    // 配置串口
    USART_InitTypeDef USART_InitStructure;
    USART_InitStructure.USART_BaudRate = 115200;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_Init(USART1, &USART_InitStructure);
    
    // 使能接收中断
    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
    
    // 配置NVIC
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    
    NVIC_InitTypeDef NVIC_InitStructure;
    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_Init(&NVIC_InitStructure);
    
    USART_Cmd(USART1, ENABLE);
}

void USART1_IRQHandler(void)
{
    static char cmd_buffer[32];
    static uint8_t cmd_index = 0;
    static uint8_t receiving_cmd = 0;
    
    if(USART_GetITStatus(USART1, USART_IT_RXNE))
    {
        char received_char = USART_ReceiveData(USART1);
        
        // 检测命令开始符 '@'
        if(received_char == '@' && !receiving_cmd)
        {
            receiving_cmd = 1;
            cmd_index = 0;
            cmd_buffer[cmd_index++] = received_char;
        }
        // 如果正在接收命令
        else if(receiving_cmd)
        {
            // 回车或换行表示命令结束
            if(received_char == '\r' || received_char == '\n') 
            {
                if(cmd_index > 0)
                {
                    cmd_buffer[cmd_index] = '\0';
                    
                    // 解析 @speed%数值 格式
                    if(strncmp(cmd_buffer, "@speed%", 7) == 0)
                    {
                        // 确保数字部分完整解析
                        char *num_str = cmd_buffer + 7;
                        if(strlen(num_str) > 0)
                        {
                            target_speed = atoi(num_str);
                            // 可选：发送确认信息用于调试
                            // printf("OK:%d\n", target_speed);
                        }
                    }
                }
                receiving_cmd = 0;
                cmd_index = 0;
            }
            else if(cmd_index < 31)
            {
                // 接收命令字符
                cmd_buffer[cmd_index++] = received_char;
                
                // 实时解析：当收到完整的 @speed% 格式时立即处理
                if(cmd_index >= 8 && strncmp(cmd_buffer, "@speed%", 7) == 0)
                {
                    // 检查是否已经收到完整的数字（以非数字字符或缓冲区满为结束）
                    char current_char = cmd_buffer[cmd_index-1];
                    if(current_char == '\r' || current_char == '\n' || 
                       current_char == ' ' || current_char == '@' ||
                       !((current_char >= '0' && current_char <= '9') || 
                         current_char == '-' || current_char == '+'))
                    {
                        // 遇到命令结束符或非法字符，立即处理之前的数字
                        char temp = cmd_buffer[cmd_index-1];
                        cmd_buffer[cmd_index-1] = '\0'; // 临时结束字符串
                        
                        char *num_str = cmd_buffer + 7;
                        if(strlen(num_str) > 0)
                        {
                            target_speed = atoi(num_str);
                        }
                        
                        // 重置接收状态，开始新的命令
                        receiving_cmd = 0;
                        cmd_index = 0;
                        
                        // 如果当前字符是新的命令开始符，立即开始新命令
                        if(temp == '@')
                        {
                            receiving_cmd = 1;
                            cmd_buffer[cmd_index++] = temp;
                        }
                    }
                    else if(cmd_index == 31)
                    {
                        // 缓冲区满，立即处理
                        cmd_buffer[cmd_index] = '\0';
                        char *num_str = cmd_buffer + 7;
                        if(strlen(num_str) > 0)
                        {
                            target_speed = atoi(num_str);
                        }
                        receiving_cmd = 0;
                        cmd_index = 0;
                    }
                }
            }
            else
            {
                // 缓冲区溢出，重置
                receiving_cmd = 0;
                cmd_index = 0;
            }
        }
        
        USART_ClearITPendingBit(USART1, USART_IT_RXNE);
    }
}
// 重定向printf
int fputc(int ch, FILE *f)
{
    USART_SendData(USART1, (uint8_t)ch);
    while(USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET);
    return ch;
}


// 发送数据到上位机 (速度1, 速度2, 位置1, 位置2)
void USART_Send_Data(int16_t speed1, int16_t speed2, long pos1, long pos2)
{
    printf("%d,%d,%ld,%ld\n", speed1, speed2, pos1, pos2);
}
