#ifndef __ENCODER_H
#define __ENCODER_H

void Encoder_Init(void);
int16_t Encoder_Get_Speed(uint8_t num);
int32_t Encoder_Get_Position(uint8_t num);
void Encoder_Clear_TotalCount(uint8_t num);

#endif
