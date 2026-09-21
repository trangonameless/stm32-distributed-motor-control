/*
 * diagnostics.c
 *
 *  Created on: 16 wrz 2026
 *      Author: patry
 */
#include "diagnostics.h"
#include "speed_sensor.h"
#include "main.h"
#include <math.h>

#include <stdio.h>
#include <stdlib.h>

#define STALL_PWM_MIN       30.0f
#define STALL_SETPOINT_MIN  30.0f
#define STALL_RPM_MAX        10.0f
#define STALL_DELAY_MS     200U
#define ENCODER_MAX_RPM          400.0f
#define ENCODER_DELAY_MS       100U
#define TRACKING_ERROR_RPM    30.0f
#define TRACKING_DELAY_MS    500U

static uint8_t faults = MOTOR_FAULT_NONE;
static uint32_t stall_start_time = 0;
static uint32_t encoder_start_time = 0;
static uint32_t tracking_start_time = 0;

void Diagnostics_Init(void)
{
    faults = MOTOR_FAULT_NONE;

    stall_start_time = 0;
    encoder_start_time = 0;
    tracking_start_time = 0;

}

static void Motor_Stall(float setpoint,
                        float measurement,
                        float pwm)
{
    if (pwm > STALL_PWM_MIN &&
        setpoint > STALL_SETPOINT_MIN &&
        measurement < STALL_RPM_MAX)
    {
        if (stall_start_time == 0)
        {
            stall_start_time = HAL_GetTick();
        }

        if (HAL_GetTick() - stall_start_time >= STALL_DELAY_MS)
        {
            faults |= MOTOR_FAULT_STALL;
        }
    }
    else
    {
        stall_start_time = 0;
    }
}


static void Encoder_Fault(float measurement)
{
    if (measurement > ENCODER_MAX_RPM)
    {
    	if (encoder_start_time == 0)
    	{
    		encoder_start_time = HAL_GetTick();
    	}
    	if (HAL_GetTick() - encoder_start_time >= ENCODER_DELAY_MS)
    	{
    		faults |= MOTOR_FAULT_ENCODER;
    	}
}
    else
    	{
    		encoder_start_time = 0;
    	}
}


static void Tracking_Error(float setpoint, float measurement)
{
    float error = setpoint - measurement;

    if (setpoint > STALL_SETPOINT_MIN &&
        error > TRACKING_ERROR_RPM)
    {
        if (tracking_start_time == 0)
        {
            tracking_start_time = HAL_GetTick();
        }

        if (HAL_GetTick() - tracking_start_time >= TRACKING_DELAY_MS)
        {
            faults |= MOTOR_TRACKING_ERROR;
        }
    }
    else
    {
        tracking_start_time = 0;
    }
}

void Diagnostics_Update(float setpoint,
                        float measurement,
                        float pwm)
{
    Motor_Stall(setpoint, measurement, pwm);
    Encoder_Fault(measurement);
    Tracking_Error(setpoint, measurement);
}

uint8_t Diagnostics_GetFaults(void)
    {
        return faults;
    }

void Diagnostics_ClearFaults(void)
    {
        faults = MOTOR_FAULT_NONE;

        stall_start_time = 0;
    }


