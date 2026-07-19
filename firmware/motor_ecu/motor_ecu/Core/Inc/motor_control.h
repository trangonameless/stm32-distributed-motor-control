/*
 * motor_control.h
 *
 *  Created on: 18 lip 2026
 *      Author: patry
 */

#ifndef INC_MOTOR_CONTROL_H_
#define INC_MOTOR_CONTROL_H_
#include <stdint.h>
#include <stdbool.h>

typedef enum
{
    MOTOR_LEFT = 0,
    MOTOR_RIGHT = 1

} MotorDirection_t;

void Motor_SetSpeed(uint8_t speed);
void Motor_SetDirection(MotorDirection_t direction);
void Motor_Stop(void);

#endif /* INC_MOTOR_CONTROL_H_ */
