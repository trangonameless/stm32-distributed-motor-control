/*
 * diagnostics.h
 *
 *  Created on: 16 wrz 2026
 *      Author: patry
 */

#ifndef INC_DIAGNOSTICS_H_
#define INC_DIAGNOSTICS_H_

#include <stdint.h>

typedef enum
{
    MOTOR_FAULT_NONE        = 0x00,
    MOTOR_FAULT_STALL       = 0x01,
    MOTOR_FAULT_OVERCURRENT = 0x02,
    MOTOR_FAULT_ENCODER     = 0x04,
    MOTOR_TRACKING_ERROR = 0x08
} MotorFault_t;

void Diagnostics_Init(void);

void Diagnostics_Update(float setpoint,
        float measurement,
        float pwm);

uint8_t Diagnostics_GetFaults(void);

void Diagnostics_ClearFaults(void);


#endif /* INC_DIAGNOSTICS_H_ */
