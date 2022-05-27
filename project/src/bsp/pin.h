/* ┌─────────────────────┐
   │ Simple GPIO control │
   └─────────────────────┘
   
    Florian Dupeyron
    May 2022
*/

#pragma once

#include <stdint.h>

#include "main.h"

/* ┌────────────────────────────────────────┐
   │ Pin def. structure                     │
   └────────────────────────────────────────┘ */

struct Pin_Def {
	GPIO_TypeDef *port;
	uint32_t       pin;
};


/* ┌────────────────────────────────────────┐
   │ Pin list                               │
   └────────────────────────────────────────┘ */

extern const struct Pin_Def pin_led;
extern const struct Pin_Def pin_vcp_rx;
extern const struct Pin_Def pin_vcp_tx;
extern const struct Pin_Def pin_nrst;

extern const struct Pin_Def pin_dmx_out;
