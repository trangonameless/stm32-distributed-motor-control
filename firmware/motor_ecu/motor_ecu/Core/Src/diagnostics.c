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

/* ---- Fault detection thresholds ---- */
#define STALL_PWM_MIN         30.0f   /* min PWM duty (%) at which stall detection is active        */
#define STALL_SETPOINT_MIN    30.0f   /* min commanded speed (RPM) at which stall detection is active */
#define STALL_RPM_MAX          10.0f  /* below this measured RPM (with high PWM/setpoint) = stalled  */
#define STALL_DELAY_MS        200U    /* debounce time before stall fault is latched                 */
#define COMM_TIMEOUT_MS   1000U /* Max time without any command/heartbeat from the supervisor (RPi)
 	 	 	 	 	 	 	 	 * while the motor is running, before the link is considered lost. */
#define ENCODER_MAX_RPM        400.0f /* measured RPM above this is physically implausible -> sensor fault */
#define ENCODER_DELAY_MS       100U   /* debounce time before encoder fault is latched                */
#define TRACKING_ERROR_RPM     30.0f  /* max allowed |setpoint - measurement| before flagging error   */
#define TRACKING_DELAY_MS      500U   /* debounce time before tracking error fault is latched         */
#define TRACKING_SETTLE_STEP_RPM   10.0f   /* setpoint change that restarts the settling window */
#define TRACKING_SETTLE_MS         3000U   /* tracking check is suspended this long after it   */
#define DRIVER_OVERCURRENT_DELAY_MS 10U

static uint8_t  faults             = MOTOR_FAULT_NONE;
static uint32_t stall_start_time    = 0;
static uint32_t encoder_start_time  = 0;
static uint32_t tracking_start_time = 0;
static uint32_t driver_overccurent_start_time = 0;
static uint32_t last_comm_tick = 0;
static float    tracking_prev_setpoint = 0.0f;
static uint32_t tracking_settle_start  = 0;
static bool     tracking_settling      = false;

void Diagnostics_Init(void)
{
    faults = MOTOR_FAULT_NONE;

    stall_start_time              = 0;
    encoder_start_time            = 0;
    tracking_start_time           = 0;
    driver_overccurent_start_time = 0;
    last_comm_tick                = HAL_GetTick();
    tracking_prev_setpoint = 0.0f;
    tracking_settling      = false;
}

/*
 * Detects a stalled motor: PWM and setpoint are both high enough that the
 * motor should be spinning, but measured speed stays near zero for longer
 * than STALL_DELAY_MS.
 */
static void Motor_Stall(float setpoint, float measurement, float pwm)
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

/*
 * Detects an implausible encoder reading (measured speed above the
 * physically achievable maximum), which points to a sensor/wiring fault
 * rather than an actual overspeed condition.
 */
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

/*
 * Detects the motor failing to track its commanded speed.
 *
 * NOTE: `measurement` is provided to this module as an absolute value
 * (see speed_sensor), so `error` here only needs to catch the
 * under-speed case (setpoint significantly above measurement). There is
 * no need for fabsf()/overshoot handling — direction is not ambiguous
 * upstream.
 */
static void Tracking_Error(float setpoint, float measurement)
{
    // A step in the setpoint (new command, motor re-enabled) needs time to
    // be followed: suspend the check while the motor is still spinning up.
    if (fabsf(setpoint - tracking_prev_setpoint) > TRACKING_SETTLE_STEP_RPM)
    {
        tracking_settling     = true;
        tracking_settle_start = HAL_GetTick();
    }
    tracking_prev_setpoint = setpoint;

    if (tracking_settling)
    {
        if (HAL_GetTick() - tracking_settle_start < TRACKING_SETTLE_MS)
        {
            tracking_start_time = 0;
            return;
        }
        tracking_settling = false;
    }

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

/*
 * Polls the DRV8876 nFAULT pin (PC8, configured as GPIO input, external
 * pull-up, no internal pull). The driver pulls this line low when its
 * internal comparator detects overcurrent or overvoltage on the motor
 * output -- this is a hardware-level protection signal
 */
static void Driver_Overcurrent(void)
{
    if (HAL_GPIO_ReadPin(Drv8876_Fault_GPIO_Port, Drv8876_Fault_Pin) == GPIO_PIN_RESET)
    {
        if (driver_overccurent_start_time == 0)
        {
        	driver_overccurent_start_time = HAL_GetTick();
        }

        if (HAL_GetTick() - driver_overccurent_start_time >= DRIVER_OVERCURRENT_DELAY_MS)
        {
            faults |= MOTOR_FAULT_OVERCURRENT;
        }
    }
    else
    {
    	driver_overccurent_start_time = 0;
    }
}

/*
 * Must be called every time a command line is received from the
 * supervisor (RPi). Refreshes the communication watchdog.
 */
void Diagnostics_CommAlive(void)
{
    last_comm_tick = HAL_GetTick();
}

/*
 * Communication watchdog: while the motor is running, the supervisor must
 * keep sending commands or heartbeats. If nothing arrives within
 * COMM_TIMEOUT_MS (script crash, unplugged cable), flag a fault so the
 * motor does not keep spinning unattended. Not checked while the motor
 * is disabled, since a stopped motor is already in a safe state.
 */
static void Comm_Timeout(bool motor_active)
{
    if (motor_active &&
        (HAL_GetTick() - last_comm_tick >= COMM_TIMEOUT_MS))
    {
        faults |= MOTOR_FAULT_COMM_TIMEOUT;
    }
}


void Diagnostics_Update(float setpoint, float measurement, float pwm, bool motor_active)
{
    Motor_Stall(setpoint, measurement, pwm);
    Encoder_Fault(measurement);
    Tracking_Error(setpoint, measurement);
//    Driver_Overcurrent();
    Comm_Timeout(motor_active);
}

uint8_t Diagnostics_GetFaults(void)
{
    return faults;
}

void Diagnostics_ClearFaults(void)
{
    faults = MOTOR_FAULT_NONE;

    stall_start_time              = 0;
    encoder_start_time            = 0;
    tracking_start_time           = 0;
    driver_overccurent_start_time = 0;
    last_comm_tick                = HAL_GetTick();
    tracking_prev_setpoint = 0.0f;
    tracking_settling      = false;
}


