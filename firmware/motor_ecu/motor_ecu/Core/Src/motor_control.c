/*
 * motor_control.c
 *
 *  Created on: 18 lip 2026
 *      Author: patry
 */
#include "motor_control.h"
#include "tim.h"

static uint8_t pwm_duty = 0;
static uint16_t set_speed = 0;
float target_rpm = 0.0f;
uint8_t motor_enabled = 0;

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


void Motor_SetSpeed(uint8_t duty)
{
    if (duty > 100)
    {
        duty = 100;
    }

    // Store current PWM duty cycle (used by telemetry)
    pwm_duty = duty;

    uint32_t pulse =
        (pwm_duty * (__HAL_TIM_GET_AUTORELOAD(&htim2) + 1)) / 100;

    __HAL_TIM_SET_COMPARE(&htim2,
                          TIM_CHANNEL_1,
                          pulse);
}

uint8_t Motor_GetPWM(void)
{
    return pwm_duty;
}

uint16_t Motor_GetSpeed(void)
{
    return set_speed;
}


void Motor_Stop(void)
{
	pwm_duty = 0;
    __HAL_TIM_SET_COMPARE(&htim2,
                          TIM_CHANNEL_1,
                          0);
}

