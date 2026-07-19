/*
 * motor_control.c
 *
 *  Created on: 18 lip 2026
 *      Author: patry
 */
#include "motor_control.h"
#include "tim.h"


void Motor_SetDirection(MotorDirection_t direction)
{
    if(direction == MOTOR_RIGHT)
    {
        HAL_GPIO_WritePin(
            MotorDirection_GPIO_Port,
            MotorDirection_Pin,
            GPIO_PIN_SET
        );
    }
    else
    {
        HAL_GPIO_WritePin(
            MotorDirection_GPIO_Port,
            MotorDirection_Pin,
            GPIO_PIN_RESET
        );
    }
}

void Motor_SetSpeed(uint8_t speed)
{
    if(speed > 100)
    {
        speed = 100;
    }


    uint32_t pulse;

    pulse = (speed * (__HAL_TIM_GET_AUTORELOAD(&htim2) + 1)) / 100;


    __HAL_TIM_SET_COMPARE(&htim2,
                          TIM_CHANNEL_1,
                          pulse);
}



void Motor_Stop(void)
{
    __HAL_TIM_SET_COMPARE(&htim2,
                          TIM_CHANNEL_1,
                          0);
}

