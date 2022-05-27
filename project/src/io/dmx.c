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


/* Switch the output pin to GPIO mode */

void __dmx_controller_switch_gpio(struct DMX_Controller *dmx)
{
	gpio_pin_init(*dmx->pin_output,
		GPIO_MODE_OUTPUT_PP,
		GPIO_NOPULL,
		GPIO_SPEED_FREQ_HIGH,
		0
	);

	/* Disable TX empty interrupt */
    ATOMIC_CLEAR_BIT(dmx->uart->CR1, USART_CR1_TCIE);
}


/* Switch the output pin to UART mode */

void __dmx_controller_switch_uart(struct DMX_Controller *dmx)
{
	gpio_pin_init(*dmx->pin_output,
		GPIO_MODE_AF_PP,
		GPIO_NOPULL,
		GPIO_SPEED_FREQ_HIGH,
		dmx->pin_uart_af
	);

	/* Enable TX empty interrupt */
    ATOMIC_SET_BIT(dmx->uart->CR1, USART_CR1_TCIE);
}

void __dmx_controller_gpio_write(struct DMX_Controller *dmx, uint8_t value)
{
	gpio_pin_write(*dmx->pin_output, value);
}

void __dmx_controller_uart_tx(struct DMX_Controller *dmx, uint8_t value)
{
	dmx->uart->TDR = value;
}

/* Init UART peripheral using HAL */
void __dmx_controller_uart_init(struct DMX_Controller *dmx)
{
	RCC_PeriphCLKInitTypeDef clk_conf = {0};

	/* Clock configure */
	clk_conf.PeriphClockSelection = RCC_PERIPHCLK_USART1;
	clk_conf.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK1;
	if (HAL_RCCEx_PeriphCLKConfig(&clk_conf) != HAL_OK)
	{
		Error_Handler();
	}

	__HAL_RCC_USART1_CLK_ENABLE();

	/* Uart configure */
	dmx->huart.Instance            = dmx->uart;
	dmx->huart.Init.BaudRate       = DMX_BAUDRATE;
	dmx->huart.Init.WordLength     = UART_WORDLENGTH_8B;
	dmx->huart.Init.StopBits       = UART_STOPBITS_2;
	dmx->huart.Init.Parity         = UART_PARITY_NONE;
	dmx->huart.Init.Mode           = UART_MODE_TX;
	dmx->huart.Init.HwFlowCtl      = UART_HWCONTROL_NONE;
	dmx->huart.Init.OverSampling   = UART_OVERSAMPLING_16;
	dmx->huart.Init.OneBitSampling = UART_ONEBIT_SAMPLING_DISABLED;

	dmx->huart.AdvancedInit.AdvFeatureInit      = UART_ADVFEATURE_DMADISABLEONERROR_INIT;
	dmx->huart.AdvancedInit.DMADisableonRxError = UART_ADVFEATURE_DMA_DISABLEONRXERROR;

	if(HAL_UART_Init(&dmx->huart) != HAL_OK) Error_Handler();

	/* Interrupts configure */
	HAL_NVIC_SetPriority(USART1_IRQn, 0, 0); // Default priority
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
		case DMX_MARK_BEFORE_BREAK:
			/* Switch output pin to TRUE */
			__dmx_controller_switch_gpio(dmx);
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
			/* Switch output pin to UART */
			__dmx_controller_switch_uart(dmx);

			/* Transmit start code */
			__dmx_controller_uart_tx(dmx, DMX_START_CODE);
			break;

		case DMX_TX_BYTE:
			/* Transmit slot data */
			__dmx_controller_uart_tx(dmx, dmx->slots[dmx->i_slot]);
			break;

		case DMX_TX_MARK:
			/* Start oneshot timer */
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
		case DMX_MARK_BEFORE_BREAK:
			if(ev == DMX_EVENT_TIMER_TIMEOUT) {
				dmx->state = DMX_START_BREAK;
			}
			break;

		case DMX_START_BREAK:
			if(ev == DMX_EVENT_TIMER_TIMEOUT) {
				dmx->state = DMX_MARK_BEFORE_BREAK; // TODO // Change
			}
			break;

		case DMX_MARK_AFTER_BREAK:
			if(ev == DMX_EVENT_TIMER_TIMEOUT) {
				dmx->state = DMX_TX_START;
			}
			break;

		case DMX_TX_START:
			if(ev == DMX_EVENT_UART_TX_DONE) {
				dmx->state = DMX_TX_BYTE;
			}
			break;

		case DMX_TX_BYTE:
			if(ev == DMX_EVENT_UART_TX_DONE) {
				dmx->state = DMX_TX_MARK;
			}
			break;

		case DMX_TX_MARK:
			if(ev == DMX_EVENT_TIMER_TIMEOUT) {
				if(dmx->i_slot >= DMX_NB_DATA_SLOTS) {
					dmx->state = DMX_UPDATE;
				}

				else {
					dmx->i_slot++;
				}
			}
			break;

		default:break;
	}

	/* Update state machine actions */
	__dmx_controller_fsm_actions(dmx);
}

/* ┌────────────────────────────────────────┐
   │ Oneshot timer callback                 │
   └────────────────────────────────────────┘ */

void __dmx_controller_oneshot_timer_done(void *usrdata)
{
	static uint8_t state = 1;

	struct DMX_Controller *dmx = (struct DMX_Controller*)usrdata;
	//__dmx_controller_event_process(dmx, DMX_EVENT_TIMER_TIMEOUT);
	
	__dmx_controller_gpio_write(dmx, state);
	state = 1 - state;

	oneshot_timer_start(60000);
}


/* ┌────────────────────────────────────────┐
   │ Public interface                       │
   └────────────────────────────────────────┘ */

void dmx_controller_init(struct DMX_Controller *dmx)
{
#if 0
	/* Init slot data */
	memset(dmx->slots   , 0, DMX_NB_DATA_SLOTS*sizeof(uint16_t));
	memset(dmx->targets , 0, DMX_NB_DATA_SLOTS*sizeof(uint8_t ));
	memset(dmx->fadetime, 0, DMX_NB_DATA_SLOTS*sizeof(uint32_t));

	/* Init state machine stuff */
	dmx->state  = DMX_MARK_BEFORE_BREAK;
	dmx->i_slot = 0;

	/* Init oneshot timer */
	oneshot_timer_init(__dmx_controller_oneshot_timer_done, (void*)dmx);

	/* Dumb slots init */
	/* TODO: Remove */
	int i_slot = DMX_NB_DATA_SLOTS;
	while(i_slot--) {
		dmx->slots[i_slot] = ((i_slot + 128) % 256);
	}
#endif
	/* Init oneshot timer */
	oneshot_timer_init(__dmx_controller_oneshot_timer_done, (void*)dmx);
}

void dmx_controller_start(struct DMX_Controller *dmx)
{
	/* Enable interrupt */
	//HAL_NVIC_EnableIRQ(USART1_IRQn);

	///* Start state machine actions */
	//__dmx_controller_fsm_actions(dmx);
	
	__dmx_controller_switch_gpio(dmx);
	oneshot_timer_start(60000);
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
