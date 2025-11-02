#ifndef __SERIAL_H
#define __SERIAL_H

#include "stm32f10x.h"

void Serial_Init(void);

void USART_Send_Data(int16_t speed1, int16_t target_speed);

#endif
