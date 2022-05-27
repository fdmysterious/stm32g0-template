/* ┌─────────────────────────────────────────────────────┐
   │ Simple DMX controller using an UART and timer stuff │
   └─────────────────────────────────────────────────────┘
   
    Florian Dupeyron
    May 2022
*/

#pragma once

#include <stdint.h>
#include <bsp/pin.h>

#include "stm32g0xx_hal.h"


/* ┌────────────────────────────────────────┐
   │ Constants                              │
   └────────────────────────────────────────┘ */

#define DMX_NB_DATA_SLOTS  512
#define DMX_START_CODE     0x00   /* NULL Start code: Dimmer packets */
#define DMX_BAUDRATE       250000 /* DMX baud rate is 250kbps*/


/* ───── Constants for various delays ───── */

//#define DMX_MBB_DELAY_US   100 /* MBB: Mark before break */
//#define DMX_BREAK_DELAY_US 100
#define DMX_MBB_DELAY_US   10000 /* MBB: Mark before break */
#define DMX_BREAK_DELAY_US 10000

#define DMX_MAB_DELAY_US   50  /* MAB: Mark after break  */
#define DMX_MARK_DELAY     50


/* ┌────────────────────────────────────────┐
   │ DMX Data                               │
   └────────────────────────────────────────┘ */

enum DMX_Controller_State {
	DMX_MARK_BEFORE_BREAK,
	DMX_START_BREAK,
	DMX_MARK_AFTER_BREAK,
	DMX_TX_START, /* TX first slot: Start code */
	DMX_TX_BYTE,
	DMX_TX_MARK,
	DMX_UPDATE
};


/* The first slot data (start code) is not stored
 * in the arrays. thus -1 for some arrays */

struct DMX_Controller {

	/* ──────────── Interface data ──────────── */

	const struct Pin_Def      *pin_output;                     /* Pin for data output         */
	uint32_t                   pin_uart_af;                     /* Alternate function for UART */

	USART_TypeDef             *uart;                            /* Used uart */
	UART_HandleTypeDef         huart;                           /* UART Handle for HAL */


	/* ────────────── Slots data ────────────── */
	
	uint16_t                   slots    [DMX_NB_DATA_SLOTS];    /* Current slot value          */
	uint8_t                    targets  [DMX_NB_DATA_SLOTS];    /* Target slot value           */

	uint32_t                   fadetime [DMX_NB_DATA_SLOTS];    /* Fade time as ms             */

	/* ─────────────── FSM data ─────────────── */

	__IO enum DMX_Controller_State  state;                     /* Current status of the FSM   */
	__IO uint32_t                   i_slot;                    /* Current slot index          */
};


/* ┌────────────────────────────────────────┐
   │ Public interface                       │
   └────────────────────────────────────────┘ */

void dmx_controller_init       (struct DMX_Controller *dmx);
void dmx_controller_start      (struct DMX_Controller *dmx);
void dmx_controller_stop       (struct DMX_Controller *dmx);

/* TODO slot set function */

void dmx_controller_irq_handler(struct DMX_Controller *dmx);
