/* ┌─────────────────────────────────────────────────────┐
   │ Simple DMX controller using an UART and timer stuff │
   └─────────────────────────────────────────────────────┘
   
    Florian Dupeyron
    May 2022
*/

#pragma once

#include <stdint.h>

/* ┌────────────────────────────────────────┐
   │ Constants                              │
   └────────────────────────────────────────┘ */

#define DMX_NBSLOTS 512


/* ┌────────────────────────────────────────┐
   │ DMX Data                               │
   └────────────────────────────────────────┘ */

enum DMX_Controller_State {
	DMX_MARK_BEFORE_BREAK,
	DMX_START_BREAK,
	DMX_MARK_AFTER_BREAK,
	DMX_TX_BYTE,
	DMX_TX_MARK,
	DMX_UPDATE
};


/* The first slot data (start code) is not stored
 * in the arrays. thus -1 for some arrays */

struct DMX_Controller {
	uint16_t slots    [DMX_NBSLOTS];   /* Current slot value        */
	uint8_t  targets  [DMX_NBSLOTS];   /* Target slot value         */

	uint32_t fadetime [DMX_NBSLOTS];   /* Fade time as ms           */

	enum DMX_Controller_State   state; /* Current status of the FSM */
};


/* ┌────────────────────────────────────────┐
   │ Public interface                       │
   └────────────────────────────────────────┘ */

void dmx_controller_init(struct DMX_Controller *dmx);
