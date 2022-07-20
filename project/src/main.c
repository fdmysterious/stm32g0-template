/* ┌────────────────────────────────────┐
   │ Simple serial based DMX controller │
   └────────────────────────────────────┘
   
    Florian Dupeyron
    May 2022
*/

#include "main.h"

#include <io/clock.h>
#include <io/oneshot_timer.h>

#include <io/gpio.h>
#include <io/uart.h>
#include <io/dmx.h>

#include <func/cmds.h>

struct DMX_Controller dmx_controller = {
	.uart        = USART1,
	.pin_output  = &pin_dmx_out,
	.pin_uart_af = GPIO_AF1_USART1
};

static char   buffer[1024];
static size_t r_len;
static struct UART_Msg_Info msg = {
	.buffer = NULL,
	.size   = 0
};

static void MX_USART2_UART_Init(void);


int hex2int(char c) {
	if     ((c >= '0') && (c <= '9')) return c - '0';
	else if((c >= 'a') && (c <= 'f')) return c - 'a';
	else if((c >= 'A') && (c <= 'F')) return c - 'A';
	else return 0;
}

int byte2int(char *v)
{
	return (hex2int(v[0])<<4)|hex2int(v[1]);
}

int main(void)
{
	HAL_Init();
	clock_init();
	
	/* GPIO Init */

	gpio_pin_init(pin_led,
		GPIO_MODE_OUTPUT_PP,
		GPIO_NOPULL,
		GPIO_SPEED_FREQ_LOW,
		0
	);

	gpio_pin_init(pin_nrst,
		GPIO_MODE_IT_RISING,
		GPIO_NOPULL,
		0,
		0
	);

	uart_init();

	/* DMX init */
	
	dmx_controller_init (&dmx_controller);

	/* Let's go! */
	
	uart_start();
	dmx_controller_start(&dmx_controller);

	while(1) {
		/* Receive message */
		do {
			msg = uart_msg_pop();
		} while(msg.buffer == NULL);

		/* Process message */

		//r_len = prpc_process_line(msg.buffer, buffer, 1023); // Keep at least one char for LF
		//gpio_pin_write(pin_led, 1);
		//if(r_len) { // if a response has been processed, r_len > 0
		//	buffer[r_len] = '\n';
		//	uart_transmit(buffer, r_len+1); // +1 for LF char
		//	msg.buffer = NULL;

		//	while(!uart_transmit_done());
		//}
		//gpio_pin_write(pin_led, 0);
	}
}

void Error_Handler(void)
{
	__disable_irq();
	while (1)
	{
		HAL_Delay(1000);
		gpio_pin_write(pin_led, 1);
		HAL_Delay(1000);
		gpio_pin_write(pin_led, 0);
	}
}

#ifdef  USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
}
#endif /* USE_FULL_ASSERT */


/* Various interrupts */

void USART1_IRQHandler(void)
{
	//static uint8_t state;
	////gpio_pin_write(pin_led, state);
	//state = 1 - state;
	dmx_controller_irq_handler(&dmx_controller);
}
