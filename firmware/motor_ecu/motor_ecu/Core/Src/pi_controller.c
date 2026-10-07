/*
 * pid_controller.c
 *
 *  Created on: 1 sie 2026
 *      Author: patry
 */


static float kp = 0.01;
static float ki = 0;
static float dt = 0.01;
static float integral = 0;

void PI_Init(float kp_value, float ki_value)
{
    kp = kp_value;
    ki = ki_value;
}


float PI_Update(float setpoint, float measurement)
{
	float error = setpoint - measurement;

	float new_integral = integral + error * dt;

	float output =
	    kp * error +
	    ki * new_integral;
    //anty wind_up
	if(output <= 100 && output >= 0)
	{
	    integral = new_integral;
	}

	if(output > 100)
	    output = 100;

	if(output < 0)
	    output = 0;


    return output;
}

void PI_Reset(void)
{
    integral = 0.0f;
}
