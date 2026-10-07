/*
 * motor_control.c
 *
 *  Created on: 18 lip 2026
 *      Author: patry
 */
#include "motor_control.h"
#include "tim.h"

static uint8_t pwm_duty = 0;
float target_rpm = 0.0f;
uint8_t motor_enabled = 0;

/*
 * Sets motor rotation direction via the H-bridge direction pin.
 * GPIO_PIN_SET   -> MOTOR_RIGHT
 * GPIO_PIN_RESET -> MOTOR_LEFT (and any other value, as default)
 * Wiring-dependent: verify against the H-bridge/motor driver datasheet.
 */
void Motor_SetDirection(MotorDirection_t direction)
{
    if (direction == MOTOR_RIGHT)
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

/*
 * Sets motor PWM duty cycle (0-100%), clamped to 100 if exceeded.
 * Computes the timer compare value proportionally to the timer's
 * auto-reload register (ARR), so it works regardless of the configured
 * PWM frequency/resolution.
 *
 * NOTE: assumes duty * (ARR + 1) fits in uint32_t, i.e. ARR well under
 * ~42 million -- true for any realistic STM32 timer configuration.
 */
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


 //Stops the motor by driving PWM duty to 0.

void Motor_Stop(void)
{
    Motor_SetSpeed(0);
}

