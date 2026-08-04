/*
 * telemetry.c
 *
 *  Created on: 22 lip 2026
 *      Author: patry
 */
#include "telemetry.h"
#include "usart.h"
#include <stdio.h>

void Telemetry_SendData(float current, float rpm, uint8_t pwm, uint16_t set_speed){
    char msg[64];

    int len = snprintf(msg, sizeof(msg),
	      "{\"current\":%.0f,\"rpm\":%.0f,\"pwm\":%u,\"set_speed\":%u}\r\n",
		  current * 1000.0f, // Convert A to mA for easier visualization in Grafana
	      rpm,
		  pwm,
		  set_speed
	  );
	      HAL_UART_Transmit(&huart1, (uint8_t*)msg, len, HAL_MAX_DELAY);
}
