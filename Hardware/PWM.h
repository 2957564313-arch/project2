#ifndef __PWM_H
#define __PWM_H

void PWM_Init(void);
void Motor_Set_Speed(uint8_t motor_num,int16_t speed);
void Motor_Process(void);

extern int32_t last_position1;
extern int32_t target_position2;

#endif
