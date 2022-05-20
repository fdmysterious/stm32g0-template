/* ┌─────────────────────┐
   │ Simple GPIO control │
   └─────────────────────┘
   
    Florian Dupeyron
    May 2022
*/

#pragma once

#include <stdint.h>

#include "main.h"
#include "pin.h"


/* ┌────────────────────────────────────────┐
   │ GPIO public interface                  │
   └────────────────────────────────────────┘ */

void  gpio_pin_init (
	struct Pin_Def   pin,

	uint32_t mode,
	uint32_t pull,
	uint32_t speed,
	uint32_t alternate
);

void    gpio_pin_write(struct Pin_Def pin, uint8_t value);
uint8_t gpio_pin_read (struct Pin_Def pin               );
