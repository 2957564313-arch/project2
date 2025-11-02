#include "stm32f10x.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "PID.h"

extern int16_t target_speed;

/**
 * @brief 串口初始化 - 115200波特率
 * 
 * 配置USART1用于与上位机通信：
 * - 发送：编码器数据和状态信息
 * - 接收：速度控制命令
 */
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
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_Init(&NVIC_InitStructure);
    
    USART_Cmd(USART1, ENABLE);
}

/**
 * @brief USART1中断服务函数 - 处理接收到的数据
 * 
 * 解析上位机发送的速度控制命令：
 * 命令格式：@speed%数值
 * 例如：@speed%10 设置目标速度为10
 */
void USART1_IRQHandler(void)
{
	// 静态变量：命令缓冲区及相关状态
    static char cmd_buffer[32];		// 命令缓冲区
    static uint8_t cmd_index = 0;	 // 缓冲区索引
    static uint8_t receiving_cmd = 0;	// 命令接收标志
    
    if(USART_GetITStatus(USART1, USART_IT_RXNE))
    {
        char received_char = USART_ReceiveData(USART1);
        
        // 检测命令开始符 '@'
        if(received_char == '@')
        {
            receiving_cmd = 1;	// 标记开始接收命令
            cmd_index = 0;		// 重置缓冲区索引
            cmd_buffer[cmd_index++] = received_char;	// 存储'@'字符
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
                    
                    // 立即解析 @speed%数值 格式
                    if(strncmp(cmd_buffer, "@speed%", 7) == 0)
                    {
                        char *num_str = cmd_buffer + 7;
                        if(strlen(num_str) > 0)
                        {
                            target_speed = atoi(num_str);
                            // 立即重置速度PID以确保快速响应
                            Speed_PID_Reset();
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

/**
 * @brief printf重定向函数
 * @param ch 要发送的字符
 * @param f 文件指针（未使用）
 * @return 发送的字符
 * 
 * 将printf输出重定向到串口，方便调试信息输出
 */
int fputc(int ch, FILE *f)
{
    USART_SendData(USART1, (uint8_t)ch);
    while(USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET);
    return ch;
}


/**
 * @brief 发送数据到上位机
 * @param speed1 电机1实际速度
 * @param target_speed 目标速度
 * 
 * 数据格式：实际速度,目标速度\n
 * 方便上位机绘制速度曲线和监控系统状态
 */
void USART_Send_Data(int16_t speed1, int16_t target_speed)
{
    printf("%d,%d\n", speed1, target_speed);
}
