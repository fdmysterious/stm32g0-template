/* ┌─────────────────────────────────┐
   │ Simple DMX controller for STM32 │
   └─────────────────────────────────┘
   Florian Dupeyron
    May 2022
*/

#include "dmx.h"

#include <memory.h>

#include <io/gpio.h>
#include <io/oneshot_timer.h>

/* ┌────────────────────────────────────────┐
   │ Private datatypes                      │
   └────────────────────────────────────────┘ */

/* Events that occurs during DMX controller process */

enum DMX_Controller_Event {
	DMX_EVENT_TIMER_TIMEOUT,
	DMX_EVENT_UART_TX_DONE,
};


/* ┌────────────────────────────────────────┐
   │ Platform-specific private interface    │
   └────────────────────────────────────────┘ */

uint32_t __dmx_controller_curtime(void)
{
	return HAL_GetTick();
}


inline void __attribute__ ((always_inline)) __dmx_controller_gpio_init(struct DMX_Controller *dmx) 
{
	gpio_pin_init(*dmx->pin_output,
		GPIO_MODE_OUTPUT_PP,
		GPIO_NOPULL,
		GPIO_SPEED_FREQ_HIGH,
		0
	);
}

/* Switch the output pin to UART mode */
inline void __attribute__ ((always_inline)) __dmx_controller_gpio_write(struct DMX_Controller *dmx, uint8_t value) 
{
	gpio_pin_write(*dmx->pin_output, value);
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
	i_slot = DMX_NB_DATA_SLOTS;
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
   │ state machine process functions        │
   └────────────────────────────────────────┘ */

/* This function manages the actions for the FSM */
/* Should be called 1 time only per state */

void __dmx_controller_fsm_actions(struct DMX_Controller *dmx)
{
	uint8_t slot_v;

	switch(dmx->state) {
		case DMX_INIT:
			__dmx_controller_gpio_init(dmx);
			oneshot_timer_start(100);
			break;

		case DMX_MARK_BEFORE_BREAK:
			/* Switch output pin to TRUE */
			__dmx_controller_gpio_write (dmx, 1);

			/* Reset slot index */
			dmx->i_slot = 0;

			/* Start oneshot timer */
			oneshot_timer_start(DMX_MBB_DELAY_US);
			
			break;

		case DMX_START_BREAK:
			/* Switch output pin to FALSE */
			__dmx_controller_gpio_write(dmx, 0);

			/* Start oneshot timer */
			oneshot_timer_start(DMX_BREAK_DELAY_US);

			break;

		case DMX_MARK_AFTER_BREAK:
			/* Switch output to true */
			__dmx_controller_gpio_write(dmx, 1);

			/* Start oneshot timer */
			oneshot_timer_start(DMX_MAB_DELAY_US);
			break;

		case DMX_TX_START:
			/* Start bit */
			if(dmx->i_bit == 0) {
				__dmx_controller_gpio_write(dmx, 0);
				oneshot_timer_start(DMX_BIT_DELAY_US);
			}

			/* Data bits */
			else if(dmx->i_bit < 9) {
				__dmx_controller_gpio_write(dmx, (DMX_START_CODE & (1<<(dmx->i_bit-1))) != 0);
				oneshot_timer_start(DMX_BIT_DELAY_US);
			}

			/* Stop bits */
			else {
				__dmx_controller_gpio_write(dmx, 1);
				oneshot_timer_start(2*DMX_BIT_DELAY_US);
			}
			break;

		case DMX_TX_START_MARK:
			__dmx_controller_gpio_write(dmx, 1);
			oneshot_timer_start(DMX_MARK_DELAY);
			break;

		case DMX_TX_BYTE:
			/* Start bit */
			if(dmx->i_bit == 0) {
				__dmx_controller_gpio_write(dmx, 0);
				oneshot_timer_start(DMX_BIT_DELAY_US);
			}

			/* Data bits */
			else if(dmx->i_bit < 9) {
				slot_v = (dmx->i_slot >> (dmx->i_bit-1)) & 0x1;
				//slot_v = ((0x55) >> (dmx->i_bit-1)) & 0x1;
				//__dmx_controller_gpio_write(dmx, (dmx->slots[dmx->i_slot] & (1<<(dmx->i_bit-1))) != 0);
				__dmx_controller_gpio_write(dmx, slot_v);
				oneshot_timer_start(DMX_BIT_DELAY_US);
			}

			/* Stop bits */
			else {
				__dmx_controller_gpio_write(dmx, 1);
				oneshot_timer_start(2*DMX_BIT_DELAY_US);
			}
			break;

		case DMX_TX_MARK:
			/* Start oneshot timer */
			__dmx_controller_gpio_write(dmx, 1);
			oneshot_timer_start(DMX_MARK_DELAY);
			break;

		case DMX_UPDATE:
			/* TODO UPDATE */
			break;

		default:break;
	}
}


/* Transitions for FSM, called on event */

void __dmx_controller_event_process(struct DMX_Controller *dmx, enum DMX_Controller_Event ev)
{
	switch(dmx->state) {
		case DMX_INIT:
			if(ev == DMX_EVENT_TIMER_TIMEOUT) {
				//dmx->state = DMX_MARK_BEFORE_BREAK;
				dmx->state = DMX_TX_START;
			}
			break;

		case DMX_MARK_BEFORE_BREAK:
			if(ev == DMX_EVENT_TIMER_TIMEOUT) {
				dmx->state = DMX_START_BREAK;
			}
			break;

		case DMX_START_BREAK:
			if(ev == DMX_EVENT_TIMER_TIMEOUT) {
				dmx->state = DMX_MARK_AFTER_BREAK;
			}
			break;

		case DMX_MARK_AFTER_BREAK:
			if(ev == DMX_EVENT_TIMER_TIMEOUT) {
				dmx->state = DMX_TX_START;
			}
			break;

		case DMX_TX_START:
			if(ev == DMX_EVENT_TIMER_TIMEOUT) {
				dmx->i_bit++;
				if(dmx->i_bit >= 10) {
					dmx->i_bit = 0;
					dmx->state = DMX_TX_BYTE;
				}
			}
			break;

		case DMX_TX_BYTE:
			if(ev == DMX_EVENT_TIMER_TIMEOUT) {
				dmx->i_bit++;
				if(dmx->i_bit >= 10) {
					dmx->i_bit = 0;
					//dmx->state = DMX_TX_MARK;

					/* TODO :: remove */
					
					dmx->i_slot++;
					if(dmx->i_slot >= DMX_NB_DATA_SLOTS) {
						dmx->lock = 1;
					}
				}
			}
			break;

		case DMX_TX_MARK:
			if(ev == DMX_EVENT_TIMER_TIMEOUT) {
				//dmx->lock = 1;

				dmx->i_slot++;
				if(dmx->i_slot >= DMX_NB_DATA_SLOTS) {
					dmx->state = DMX_UPDATE;
				}

				else {
					dmx->state = DMX_TX_BYTE;
				}
			}
			break;

		default:break;
	}

	/* Update state machine actions */
	if(!dmx->lock) __dmx_controller_fsm_actions(dmx);
}

/* ┌────────────────────────────────────────┐
   │ Oneshot timer callback                 │
   └────────────────────────────────────────┘ */

void __dmx_controller_oneshot_timer_done(void *usrdata)
{
	struct DMX_Controller *dmx = (struct DMX_Controller*)usrdata;
	__dmx_controller_event_process(dmx, DMX_EVENT_TIMER_TIMEOUT);
}


/* ┌────────────────────────────────────────┐
   │ Public interface                       │
   └────────────────────────────────────────┘ */

void dmx_controller_init(struct DMX_Controller *dmx)
{
	/* Init slot data */
	memset(dmx->slots   , 0, DMX_NB_DATA_SLOTS*sizeof(uint16_t));
	memset(dmx->targets , 0, DMX_NB_DATA_SLOTS*sizeof(uint8_t ));
	memset(dmx->fadetime, 0, DMX_NB_DATA_SLOTS*sizeof(uint32_t));

	/* Init state machine stuff */
	dmx->state  = DMX_INIT;
	dmx->i_slot = 0;
	dmx->i_bit  = 0;

	/* Init oneshot timer */
	oneshot_timer_init(__dmx_controller_oneshot_timer_done, (void*)dmx);

	/* Dumb slots init */
	/* TODO: Remove */
	int i_slot = DMX_NB_DATA_SLOTS;
	while(i_slot--) {
		dmx->slots[i_slot] = ((i_slot + 128) % 256);
	}

	dmx->lock = 0;
}

void dmx_controller_start(struct DMX_Controller *dmx)
{
	/* Start state machine actions */
	__dmx_controller_fsm_actions(dmx);
}


/* ┌────────────────────────────────────────┐
   │ IRQ Handler                            │
   └────────────────────────────────────────┘ */

void dmx_controller_irq_handler(struct DMX_Controller *dmx)
{
	/* Check interrupts for UART */
	uint32_t isrflags   = READ_REG(dmx->uart->ISR);

	/* Transfer complete interrupt */
	if(isrflags & USART_ISR_TC) {
		__dmx_controller_event_process(dmx, DMX_EVENT_UART_TX_DONE);
	}
}
