/*
 * speed_sensor.c
 *
 *  Created on: 20 lip 2026
 *      Author: patry
 */

#include "speed_sensor.h"
#include "tim.h"

#define ENCODER_COUNTS_PER_REV             924.0f //462.0f       // Waveshare 22346 datasheet
#define UPDATE_PERIOD_MS       10.0f

static int32_t encoder_position = 0;
static int32_t previous_position = 0;

static float motor_rpm = 0.0f;

static uint32_t previous_tick = 0;


void SpeedSensor_Init(void)
{
    HAL_TIM_Encoder_Start(&htim3, TIM_CHANNEL_ALL);

    encoder_position = __HAL_TIM_GET_COUNTER(&htim3);
    previous_position = encoder_position;

    previous_tick = HAL_GetTick();
}

void SpeedSensor_Update(void)
{
    uint32_t current_tick = HAL_GetTick();

    if ((current_tick - previous_tick) >= UPDATE_PERIOD_MS)
    {
        previous_tick = current_tick;

        encoder_position = __HAL_TIM_GET_COUNTER(&htim3);

        int32_t delta = encoder_position - previous_position;

        previous_position = encoder_position;

        motor_rpm =
            ((float)delta / ENCODER_COUNTS_PER_REV)
            * (60000.0f / UPDATE_PERIOD_MS);
    }
}

int32_t SpeedSensor_GetPosition(void)
{
    return encoder_position;
}

float SpeedSensor_GetRPM(void)
{
    return motor_rpm;
}
