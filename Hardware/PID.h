#ifndef __PID_H
#define __PID_H

#include "stm32f10x.h"

// PID参数设置
void Speed_PID_SetParams(float p, float i, float d);
void Position_PID_SetParams(float p, float i, float d);

// PID计算
int16_t Speed_PID_Calculate(int16_t target, int16_t actual);
int16_t Position_PID_Calculate(int32_t target, int32_t actual);

// PID重置
void Speed_PID_Reset(void);

#endif
