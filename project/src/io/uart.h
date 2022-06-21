/* ┌───────────────────────┐
   │ Simple UART managment │
   └───────────────────────┘
   
    Florian Dupeyron
    March 2022

	→ For the UART module, the LL driver is used instead of HAL
*/

#pragma once

#define UART_INPUT_SIZE_POW 10
#define UART_INPUT_SIZE     (1<<UART_INPUT_SIZE_POW)  // 1024
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
