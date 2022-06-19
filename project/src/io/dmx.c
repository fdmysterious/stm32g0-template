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
	DMX_EVENT_UART_TX_BREAK
};


/* ┌────────────────────────────────────────┐
   │ Platform-specific private interface    │
   └────────────────────────────────────────┘ */

uint32_t __dmx_controller_curtime(void)
{
	return HAL_GetTick();
}


/* ───────────────── GPIO ───────────────── */

inline void __attribute__ ((always_inline)) __dmx_controller_gpio_init(struct DMX_Controller *dmx) 
{
	/* Init as AF pin */
	gpio_pin_init(*dmx->pin_output,
		GPIO_MODE_AF_PP,
		GPIO_PULLDOWN,
		GPIO_SPEED_FREQ_HIGH,
		dmx->pin_uart_af
	);
}

inline void __attribute__ ((always_inline)) __dmx_controller_do_mark(struct DMX_Controller *dmx)
{
	/* The trick is to enable the transmitter but transmit no data, to
	   toggle the output to the idle level (high) */
	// Nothing to do!
}

inline void __attribute__ ((always_inline)) __dmx_controller_do_space(struct DMX_Controller *dmx)
{
	/* Here we generate a break call! */
	__HAL_UART_SEND_REQ(&dmx->huart, UART_SENDBREAK_REQUEST);
}

/* ───────────────── UART ───────────────── */

void __dmx_controller_uart_init(struct DMX_Controller *dmx)
{
	/* Init clock */
	RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};
	
	PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART1;
	PeriphClkInit.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK1;
	if(HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK) Error_Handler();

	__HAL_RCC_USART1_CLK_ENABLE();
	
	/* HAL Init */
	dmx->huart.Instance = dmx->uart;

	dmx->huart.Init.BaudRate       = DMX_BAUDRATE;
	dmx->huart.Init.WordLength     = UART_WORDLENGTH_8B;
	dmx->huart.Init.StopBits       = UART_STOPBITS_2;
	dmx->huart.Init.Parity         = UART_PARITY_NONE;
	dmx->huart.Init.Mode           = UART_MODE_TX;
	dmx->huart.Init.HwFlowCtl      = UART_HWCONTROL_NONE;
	dmx->huart.Init.OverSampling   = UART_OVERSAMPLING_16;
	dmx->huart.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
	dmx->huart.Init.ClockPrescaler = UART_PRESCALER_DIV1;

	dmx->huart.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;

	if(HAL_UART_Init   (&dmx->huart)                                         != HAL_OK) Error_Handler();
	if(HAL_UARTEx_SetTxFifoThreshold(&dmx->huart, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK) Error_Handler();
	if(HAL_UARTEx_DisableFifoMode(&dmx->huart)                               != HAL_OK) Error_Handler();

	ATOMIC_SET_BIT(dmx->uart->CR1, USART_CR1_TCIE);

	/* IRQ configure */
	HAL_NVIC_SetPriority(USART1_IRQn, 0, 0);
	
	/* Enable Transmit complete interruption */
	HAL_NVIC_EnableIRQ  (USART1_IRQn);
}

static void __dmx_controller_uart_tx(struct DMX_Controller *dmx, uint32_t data)
{
	dmx->uart->TDR = data;
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
	switch(dmx->state) {
		case DMX_INIT:
			oneshot_timer_start(1000);
			break;

		case DMX_MARK_BEFORE_BREAK:
			/* Switch output pin to TRUE */
			__dmx_controller_do_mark(dmx);

			/* Reset slot index */
			dmx->i_slot = 0;

			/* Start oneshot timer */
			oneshot_timer_start(DMX_MBB_DELAY_US);
			
			break;

		case DMX_START_BREAK:
			/* Switch output pin to FALSE */
			__dmx_controller_do_space(dmx);
			//__dmx_controller_uart_tx(dmx, 0x00);

			/* Start oneshot timer */
			oneshot_timer_start(DMX_BREAK_DELAY_US);

			break;

		case DMX_MARK_AFTER_BREAK:
			/* Switch output to true */
			__dmx_controller_do_mark(dmx);

			/* Start oneshot timer */
			oneshot_timer_start(DMX_MAB_DELAY_US);
			break;

		case DMX_TX_START:
			__dmx_controller_uart_tx(dmx, 0x00); // Start code
			break;

		case DMX_TX_START_MARK:
			/* Line level is already high, so wait for the mark delay*/
			oneshot_timer_start(DMX_MARK_DELAY);
			break;

		case DMX_TX_BYTE:
			/* TODO Shift the value */
			__dmx_controller_uart_tx(dmx, dmx->slots[dmx->i_slot]);
			break;

		case DMX_TX_MARK:
			/* Line level is already high, so wait for the mark delay*/
			oneshot_timer_start(DMX_MARK_DELAY);
			break;

		case DMX_UPDATE:
			/* TODO UPDATE */
			/* For now, just trigger the timer */
			
			dmx->state = DMX_MARK_BEFORE_BREAK;
			__dmx_controller_fsm_actions(dmx);
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
				dmx->state = DMX_MARK_BEFORE_BREAK;
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
			if(ev == DMX_EVENT_UART_TX_DONE) {
				if(DMX_MARK_DELAY == 0) {
					dmx->state = DMX_TX_BYTE;
				}

				else {
					dmx->state = DMX_TX_START_MARK;
				}
			}
			break;

		case DMX_TX_START_MARK:
			if(ev == DMX_EVENT_TIMER_TIMEOUT) {
				dmx->state = DMX_TX_BYTE;
			}
			break;

		case DMX_TX_BYTE:
			if(ev == DMX_EVENT_UART_TX_DONE) {
				/* Increase slot index, check against last slot */
				dmx->i_slot++;
				if(dmx->i_slot >= DMX_NB_DATA_SLOTS) {
					dmx->state = DMX_UPDATE;
				}

				else {
					/* If no MARK delay, directly transmit another byte */
					if(DMX_MARK_DELAY == 0) {
						dmx->state = DMX_TX_BYTE;
					}

					/* Else, trigger the MARK state */
					else {
						dmx->state = DMX_TX_MARK;
					}
				}
			}
			break;

		case DMX_TX_MARK:
			if(ev == DMX_EVENT_TIMER_TIMEOUT) {
				/* Transmit next slot data */
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

	/* Init UART and GPIO */
	__dmx_controller_uart_init(dmx);
	__dmx_controller_gpio_init(dmx);

	/* Dumb slots init */
	/* TODO: Remove */
	//int i_slot = DMX_NB_DATA_SLOTS;
	//while(i_slot--) {
	//	//dmx->slots[i_slot] = (0x80+i_slot)&0xFF;
	//	dmx->slots[i_slot] = i_slot;
	//}
	
	dmx->slots[0] = 125; // Level operation
	dmx->slots[1] = 0;   // Level fine tuning
	dmx->slots[2] = 28;  // Vertical operation
	dmx->slots[3] = 0;   // Vertical trimming
	dmx->slots[4] = 160; // Color: Automatic color change
	dmx->slots[5] = 1;   // Fix spot
	dmx->slots[6] = 0;   // Strobe
	dmx->slots[7] = 128; // Dimming
	dmx->slots[8] = 128; // Move speed
	dmx->slots[9] = 0;   // No auto mode
	dmx->slots[10] = 0;  // No reset


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
		dmx->uart->ICR = USART_ICR_TCCF; // Clear interrupt flag
	}
}
