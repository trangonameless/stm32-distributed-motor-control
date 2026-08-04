/*
 * pid_controller.h
 *
 *  Created on: 1 sie 2026
 *      Author: patry
 */
#pragma once
#ifndef INC_PID_CONTROLLER_H_
#define INC_PID_CONTROLLER_H_


float PID_Update(float setpoint, float measurement);
void PID_Init(float kp, float ki, float kd);
void PID_Reset(void);

#endif


