/*
 * speed_sensor.h
 *
 *  Created on: 20 lip 2026
 *      Author: patry
 */

#ifndef INC_SPEED_SENSOR_H_
#define INC_SPEED_SENSOR_H_
#include <stdint.h>


void SpeedSensor_Init(void);
void SpeedSensor_Update(void);


int32_t SpeedSensor_GetPosition(void);
float SpeedSensor_GetRPM(void);



#endif /* INC_SPEED_SENSOR_H_ */
