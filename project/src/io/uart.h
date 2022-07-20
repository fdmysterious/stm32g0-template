/* ┌───────────────────────┐
   │ Simple UART managment │
   └───────────────────────┘
   
    Florian Dupeyron
    March 2022

	→ For the UART module, the LL driver is used instead of HAL

	The used protocol here is quite straightforward:

	7      07     1 7        07        07       0
	| byte1 | byte2 | byte3   | byte4   | byte5 |
	| STX   | CMD | VALUE 0   | VALUE 1 | ETX   |

	This module only receives commands between STX and ETX characters.


	TODO: This description will be moved in the proper module, it is here as a
	memory note.

	Commands are:
	-------------
	0: Channel set -> VALUE0 is channel, value1 is channel value
	1: Blackout    -> VALUE0 is 0 or 1 if blackout is enabled or not.
*/

#pragma once

#define CHAR_STX 2
#define CHAR_ETX 3

#define UART_INPUT_SIZE_POW 3 // Input buffer size is 8, command size should be 5
#define UART_INPUT_SIZE     (1<<UART_INPUT_SIZE_POW)
#define UART_BAUDRATE        921600

#include "stm32g0xx_hal.h"

/* ┌────────────────────────────────────────┐
   │ Structures                             │
   └────────────────────────────────────────┘ */

struct UART_Msg_Info {
	char*  buffer; // NULL if no message is available
	size_t   size;
};


/* ┌────────────────────────────────────────┐
   │ Public interface                       │
   └────────────────────────────────────────┘ */

void                 uart_init   (void);
void                 uart_start  (void);
struct UART_Msg_Info uart_msg_pop(void);

uint8_t uart_transmit(const char *buffer, size_t size);
uint8_t uart_transmit_done(void);
