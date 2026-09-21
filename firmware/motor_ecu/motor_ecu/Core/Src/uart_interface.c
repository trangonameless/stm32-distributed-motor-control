/*
 * uart_interface.c
 *
 *  Created on: 2 sie 2026
 *      Author: patry
 */
#include "motor_control.h"
#include <stdlib.h>
#include <pid_controller.h>
#include <stdio.h>
#include <diagnostics.h>

#define LINE_MAX_LENGTH	80
static char line_buffer[LINE_MAX_LENGTH + 1];
static uint32_t line_length;

void Process_Command(char *command)
{
    char direction;

    direction = command[0];

    switch(direction)
    {
        case 'L':

            Motor_SetDirection(MOTOR_LEFT);

            target_rpm = atof(&command[1]);

            printf("LEFT %.0f RPM\r\n", target_rpm);
            motor_enabled = 1;

            break;

        case 'P':

            Motor_SetDirection(MOTOR_RIGHT);

            target_rpm = atof(&command[1]);

            printf("RIGHT %.0f RPM\r\n", target_rpm);
            motor_enabled = 1;

            break;

        case 'S':

        	motor_enabled = 0;
            target_rpm = 0;

            Motor_Stop();
            PID_Reset();

            printf("STOP\r\n");


            break;

        case 'R':

        	Diagnostics_ClearFaults();
            printf("Alarm RESET\r\n");


            break;

        default:

            printf("Unknown command\r\n");

            break;
    }
}

void line_append(uint8_t value)
{
	if (value == '\r' || value == '\n')
	{
	    if(line_length > 0)
	    {
	        line_buffer[line_length] = '\0';

	        Process_Command(line_buffer);

	        line_length = 0;
	    }
	}
	else
	{
	    if(line_length < LINE_MAX_LENGTH)
	    {
	        line_buffer[line_length++] = value;
	    }
	    else
	    {
	        line_length = 0;
	    }
	}
}
