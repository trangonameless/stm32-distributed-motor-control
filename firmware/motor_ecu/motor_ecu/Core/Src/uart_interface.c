/*
 * uart_interface.c
 *
 *  Created on: 2 sie 2026
 *      Author: patry
 */
#include "motor_control.h"
#include <stdlib.h>
#include <stdio.h>
#include <diagnostics.h>
#include <pi_controller.h>
#include "uart_interface.h"
#include "string.h"

/* Motor's rated maximum speed per datasheet -- used to clamp UART-supplied
 * setpoints so the controller is never commanded beyond what the motor
 * can physically do. */
#define MOTOR_MAX_RPM   240.0f

/*
 * Simple line-buffered UART command interface.
 *
 * Bytes arrive one at a time from the USART1 RX interrupt
 * (UART_Command_ReceiveByte, called from HAL_UART_RxCpltCallback) and are
 * assembled into a line. Once a full line ('\r' or '\n') is received, it
 * is copied into command_buffer and command_ready is set, for the main
 * loop to pick up via UART_Command_Get() and dispatch via Process_Command().
 *
 * Producer/consumer design note: command_buffer is only overwritten by
 * the ISR while command_ready is false, i.e. only after the main loop has
 * consumed the previous command. This avoids the ISR ever mutating
 * command_buffer while UART_Command_Get() is mid-copy from it -- no extra
 * locking/critical section is needed. Trade-off: if a new line completes
 * before the previous command has been consumed, that new command is
 * silently dropped (first-received command wins, not the latest).
 */

static char line_buffer[LINE_MAX_LENGTH + 1];
static char command_buffer[LINE_MAX_LENGTH + 1];

static uint8_t line_length = 0;
static volatile bool command_ready = false;

/*
 * Dispatches a single-character command line, with an optional numeric
 * argument following the command letter.
 *
 *   L<rpm>  - spin left,  set target speed to <rpm> (clamped to 0..MOTOR_MAX_RPM)
 *   P<rpm>  - spin right, set target speed to <rpm> (clamped to 0..MOTOR_MAX_RPM)
 *   S       - stop motor, disable control, reset PI integrator
 *   R       - clear latched diagnostic faults
 *
 * Malformed/missing numeric argument (e.g. bare "L") is handled safely:
 * atof() on an empty/invalid string returns 0.0.
 */
void Process_Command(const char *command)
{
    char function_command;

    function_command = command[0];

    switch (function_command)
    {
        case 'L':

            Motor_SetDirection(MOTOR_LEFT);

            target_rpm = atof(&command[1]);

            // Guard against negative/garbage setpoints (e.g. malformed
            // command, or bare 'L' with no number -> atof returns 0.0,
            // which is fine; a negative value would otherwise wrap to a
            // huge number when later cast to uint16_t for telemetry).
            if (target_rpm < 0.0f)
            {
                target_rpm = 0.0f;
            }
            // Clamp to the motor's rated maximum (datasheet: 240 RPM) --
            // prevents an over-range setpoint from being handed to the PI
            // controller and diagnostics.
            else if (target_rpm > MOTOR_MAX_RPM)
            {
                target_rpm = MOTOR_MAX_RPM;
            }

            motor_enabled = 1;

            break;

        case 'P':

            Motor_SetDirection(MOTOR_RIGHT);

            target_rpm = atof(&command[1]);

            if (target_rpm < 0.0f)
            {
                target_rpm = 0.0f;
            }
            else if (target_rpm > MOTOR_MAX_RPM)
            {
                target_rpm = MOTOR_MAX_RPM;
            }

            motor_enabled = 1;

            break;

        case 'S':

            motor_enabled = 0;
            target_rpm = 0;

            Motor_Stop();
            PI_Reset();

            break;

        case 'R':

            Diagnostics_ClearFaults();

            break;

        default:

            printf("Unknown command\r\n");

            break;
    }
}

/*
 * Called from the USART1 RX interrupt, once per received byte.
 * Accumulates bytes into line_buffer until a line terminator ('\r' or
 * '\n') is seen, then hands the completed line off to command_buffer
 * for the main loop (see design note above). Runs in interrupt context:
 * must stay fast and must not block.
 */
void UART_Command_ReceiveByte(uint8_t value)
{
    if (value == '\r' || value == '\n')
    {
        if (line_length > 0)
        {
            line_buffer[line_length] = '\0';

            if (!command_ready)
            {
                strcpy(command_buffer, line_buffer);
                command_ready = true;
            }
            // else: previous command not yet consumed -- this line is
            // dropped (see design note above).

            line_length = 0;
        }
    }
    else
    {
        if (line_length < LINE_MAX_LENGTH)
        {
            line_buffer[line_length++] = value;
        }
        else
        {
            // Line too long: discard and start over rather than overflow.
            line_length = 0;
        }
    }
}

bool UART_Command_Available(void)
{
    return command_ready;
}

/*
 * Copies the pending command into destination (must be at least
 * LINE_MAX_LENGTH + 1 bytes) and clears the ready flag.
 * Returns false without modifying destination if no command is pending.
 */
bool UART_Command_Get(char *destination)
{
    if (!command_ready)
        return false;

    strcpy(destination, command_buffer);
    command_ready = false;

    return true;
}
