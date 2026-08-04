/*
 * telemetry.h
 *
 *  Created on: 22 lip 2026
 *      Author: patry
 */

#ifndef INC_TELEMETRY_H_
#define INC_TELEMETRY_H_
#include <stdint.h>

void Telemetry_SendData(float current, float rpm, uint8_t pwm, uint16_t set_speed);

#endif /* INC_TELEMETRY_H_ */
