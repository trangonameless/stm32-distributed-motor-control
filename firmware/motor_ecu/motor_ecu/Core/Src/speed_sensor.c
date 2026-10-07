/*
 * speed_sensor.c
 *
 *  Created on: 20 lip 2026
 *      Author: patry
 */

#include "speed_sensor.h"
#include "tim.h"


#define ENCODER_COUNTS_PER_REV   924.0f

#define UPDATE_PERIOD_MS         10U    // RPM sampling/update period

static uint16_t previous_count = 0;     // raw TIM3 counter value at last update
static float    motor_rpm      = 0.0f;

static uint32_t previous_tick  = 0;

void SpeedSensor_Init(void)
{
    HAL_TIM_Encoder_Start(&htim3, TIM_CHANNEL_ALL);

    previous_count = (uint16_t)__HAL_TIM_GET_COUNTER(&htim3);
    previous_tick  = HAL_GetTick();
}

/*
 * Samples the encoder counter every UPDATE_PERIOD_MS and converts the
 * count delta into RPM.
 *
 * Wrap-around handling: TIM3's counter register is 16-bit and rolls over
 * every 65536 counts (~70 revolutions at 924 counts/rev). The delta is
 * computed via a 16-bit unsigned subtraction, then reinterpreted as a
 * signed 16-bit value -- two's complement arithmetic makes this correct
 * across a rollover in either direction, as long as the actual movement
 * between samples stays under half the counter range (32768 counts),
 * which holds comfortably at a 10ms sampling period for any realistic
 * motor speed.
 */
void SpeedSensor_Update(void)
{
    uint32_t current_tick = HAL_GetTick();

    if ((current_tick - previous_tick) >= UPDATE_PERIOD_MS)
    {
        previous_tick = current_tick;

        uint16_t current_count = (uint16_t)__HAL_TIM_GET_COUNTER(&htim3);

        // Wrap-safe delta: subtract in the counter's native width (16-bit),
        // then reinterpret the result as signed.
        int16_t delta16 = (int16_t)(current_count - previous_count);
        int32_t delta   = (int32_t)delta16;

        previous_count = current_count;

        motor_rpm =
            ((float)delta / ENCODER_COUNTS_PER_REV)
            * (60000.0f / (float)UPDATE_PERIOD_MS);
    }
}

/*
 * Returns the raw current TIM3 counter value (0-65535), NOT an
 * accumulated absolute position -- it wraps every revolution of the
 * counter itself. Suitable for debugging/diagnostics; if a true
 * multi-revolution absolute position is ever needed, this would need
 * a separate accumulator that tracks full wrap-arounds.
 */
uint16_t SpeedSensor_GetPosition(void)
{
    return previous_count;
}

float SpeedSensor_GetRPM(void)
{
    return motor_rpm;
}



