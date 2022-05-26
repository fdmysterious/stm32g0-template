/* ┌─────────────────────────────────┐
   │ Simple DMX controller for STM32 │
   └─────────────────────────────────┘
   
    Florian Dupeyron
    May 2022
*/

#include "dmx.h"
#include <memory.h>

/* ┌────────────────────────────────────────┐
   │ Platform-specific private interface    │
   └────────────────────────────────────────┘ */

uint32_t __dmx_controller_curtime(void)
{
	return 0;
}

/* ┌────────────────────────────────────────┐
   │ Private interface                      │
   └────────────────────────────────────────┘ */

/* Updates the current values */
/* delta_ms is the time difference since last update. */

void __dmx_controller_update(struct DMX_Controller *dmx, uint32_t delta_ms)
{
	uint64_t tmp;
	uint64_t  v1;
	uint64_t  v2;

	int   i_slot;

	/* Slot data is stored as q8 values. */
	i_slot = DMX_NBSLOTS;
	while(i_slot--) {
		v2   = (uint64_t)(dmx->targets[i_slot])<<8;
		v1   = (uint64_t)(dmx->slots[i_slot]);

		tmp  = (v2-v1);
		tmp *= delta_ms;
		tmp /= dmx->fadetime[i_slot];

		dmx->slots[i_slot]     = (uint16_t)(tmp & 0xFFFF);
		dmx->fadetime[i_slot] -= delta_ms;
	}
}

/* ┌────────────────────────────────────────┐
   │ Public interface                       │
   └────────────────────────────────────────┘ */

void dmx_controller_init(struct DMX_Controller *dmx)
{
	dmx->state = DMX_MARK_BEFORE_BREAK;
	
	memset(dmx->slots   , 0, DMX_NBSLOTS*sizeof(uint16_t));
	memset(dmx->targets , 0, DMX_NBSLOTS*sizeof(uint8_t ));
	memset(dmx->fadetime, 0, DMX_NBSLOTS*sizeof(uint32_t));
}
