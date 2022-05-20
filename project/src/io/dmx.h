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

#define DMX_NBSLOTS 513


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


struct DMX_Controller {
	uint8_t  slots    [DMX_NBSLOTS];
	uint8_t  targets  [DMX_NBSLOTS];

	// fade time as milliseconds
	uint32 fadetime [DMX_NBSLOTS]; // 4*513
};


/* ┌────────────────────────────────────────┐
   │ Public interface                       │
   └────────────────────────────────────────┘ */

void dmx_controller_init(struct DMX_Controller *dmx);
void dmx_controller_slot_set();
