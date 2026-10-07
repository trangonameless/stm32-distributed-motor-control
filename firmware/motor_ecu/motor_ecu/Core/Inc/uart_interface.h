/*
 * uart_interface.h
 *
 *  Created on: 2 sie 2026
 *      Author: patry
 */

#ifndef INC_UART_INTERFACE_H_
#define INC_UART_INTERFACE_H_

#define LINE_MAX_LENGTH 32



void Process_Command(const char *command);

void UART_Command_ReceiveByte(uint8_t value);

bool UART_Command_Available(void);

bool UART_Command_Get(char *destination);




#endif /* INC_UART_INTERFACE_H_ */
