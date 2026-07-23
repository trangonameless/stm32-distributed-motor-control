/*
 * current_sense.c
 *
 *  Created on: 19 lip 2026
 *      Author: patry
 */
#include "current_sense.h"
#include "main.h"
#include "adc.h"
#include <stdio.h>

#define RIPROPI_OHMS        2470.0f   // Measured on PCB: CS to GND
#define AIPROPI_uA_PER_A    1000.0f  // DRV8876 datasheet


void CurrentSense_Init(void)
{
	HAL_ADCEx_Calibration_Start(&hadc1, ADC_SINGLE_ENDED);
	HAL_ADC_Start(&hadc1);
}

float CurrentSense_ReadCurrent(void)
{
	uint32_t value = HAL_ADC_GetValue(&hadc1);
	float voltage = 3.3f * value / 4095.0f;
	 /*
	     * DRV8876 current sense:
	     *
	     * IIPROPI = IMOTOR * AIPROPI
	     * VIPROPI = IIPROPI * RIPROPI
	     *
	     * For this board:
	     * RIPROPI = 2470 ohm
	     * AIPROPI = 1000 uA/A
	     *
	     * Therefore:
	     * IMOTOR = VIPROPI / 2.47
	     */
	float current =
	        voltage /
	        (RIPROPI_OHMS * (AIPROPI_uA_PER_A / 1000000.0f));

	return current;
}
