/*
 * pid_controller.h
 *
 *  Created on: 1 sie 2026
 *      Author: patry
 */
#pragma once
#ifndef INC_PI_CONTROLLER_H_
#define INC_PI_CONTROLLER_H_


float PI_Update(float setpoint, float measurement);
void PI_Init(float kp, float ki);
void PI_Reset(void);

#endif


