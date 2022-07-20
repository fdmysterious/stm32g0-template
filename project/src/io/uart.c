/* ┌───────────────────────┐
   │ Simple UART managment │
   └───────────────────────┘
   
    Florian Dupeyron
    March 2022

	For this UART driver, the HAL is used only for init. RX/TX processing is done using
	raw peripheral madness.
*/

#include "uart.h"
#include "stm32g0xx_hal.h"
#include "gpio.h"

#include <memory.h>
#include <io/gpio.h>

/* ┌────────────────────────────────────────┐
   │ Data struct                            │
   └────────────────────────────────────────┘ */

/* Simple scheme: Input is double buffered */
struct UART_Data {
	/* ──────────────── RX data ─────────────── */
	
	char                rx_buffer[UART_INPUT_SIZE<<1]; // Read buffer
	volatile size_t     rx_side;                       // Current reading buffer

	volatile size_t     rx_idx;                        // Current read index
	volatile size_t     rx_size;                       // Last read message size

	volatile uint8_t    rx_flag;                       // A message is available for processing


	/* ──────────────── TX data ─────────────── */
	
	const char         *tx_buffer;                     // Pointer to buffer to transmit
	volatile size_t     tx_size;                       // Remaining words to transmit
	volatile size_t     tx_idx;                        // Current index in TX buffer
	volatile uint8_t    tx_expect_txe;                 // Waiting for TXE interrupt ?
};

/* ┌────────────────────────────────────────┐
   │ Static data                            │
   └────────────────────────────────────────┘ */

static UART_HandleTypeDef    huart2;
static struct UART_Data   uart_data;

/* ┌────────────────────────────────────────┐
   │ Private interface                      │
   └────────────────────────────────────────┘ */

static void uart_tx_next()
{
	if(uart_data.tx_buffer == NULL) return; // Guard against null buffers

	huart2.Instance->TDR    = uart_data.tx_buffer[uart_data.tx_idx];
	uart_data.tx_expect_txe = 1;
}

/* ┌────────────────────────────────────────┐
   │ Public interface                       │
   └────────────────────────────────────────┘ */

void uart_init(void)
{
	//RCC_PeriphCLKInitTypeDef clk_conf = {0};
	GPIO_InitTypeDef         gpio_conf;

	///* Clock configure */
	//clk_conf.PeriphClockSelection = RCC_PERIPHCLK_USART2;
	//clk_conf.Usart2ClockSelection = RCC_USART2CLKSOURCE_PCLK1;
	//if (HAL_RCCEx_PeriphCLKConfig(&clk_conf) != HAL_OK)
	//{
	//	Error_Handler();
	//}
	
	__HAL_RCC_USART2_CLK_ENABLE();
	__HAL_RCC_GPIOA_CLK_ENABLE();

	/* GPIO configure */

	/* → PIN_USART_TX */
	gpio_conf.Pin              = pin_vcp_tx.pin;
	gpio_conf.Mode             = GPIO_MODE_AF_PP;
	gpio_conf.Pull             = GPIO_NOPULL;
	gpio_conf.Speed            = GPIO_SPEED_FREQ_HIGH;
	gpio_conf.Alternate        = GPIO_AF1_USART2;
	HAL_GPIO_Init(pin_vcp_tx.port, &gpio_conf);
	
	/* → PIN_USART_RX */
	gpio_conf.Pin              = pin_vcp_rx.pin;
	gpio_conf.Mode             = GPIO_MODE_AF_PP;
	gpio_conf.Pull             = GPIO_NOPULL;
	gpio_conf.Speed            = GPIO_SPEED_FREQ_HIGH;
	gpio_conf.Alternate        = GPIO_AF1_USART2;
	HAL_GPIO_Init(pin_vcp_rx.port, &gpio_conf);

	/* Peripheral configure */
	huart2.Instance            = USART2;
	huart2.Init.BaudRate       = UART_BAUDRATE;
	huart2.Init.WordLength     = UART_WORDLENGTH_8B;
	huart2.Init.StopBits       = UART_STOPBITS_1;
	huart2.Init.Parity         = UART_PARITY_NONE;
	huart2.Init.Mode           = UART_MODE_TX_RX;
	huart2.Init.HwFlowCtl      = UART_HWCONTROL_NONE;
	huart2.Init.OverSampling   = UART_OVERSAMPLING_16;
	huart2.Init.OneBitSampling = UART_ONEBIT_SAMPLING_DISABLED;
	huart2.AdvancedInit.AdvFeatureInit      = UART_ADVFEATURE_DMADISABLEONERROR_INIT;
	huart2.AdvancedInit.DMADisableonRxError = UART_ADVFEATURE_DMA_DISABLEONRXERROR;

	if(HAL_UART_Init(&huart2) != HAL_OK) Error_Handler();

	/* Interrupts configure */
	HAL_NVIC_SetPriority(USART2_IRQn, 0, 0); // Default priority

	/* Data init */
	memset(uart_data.rx_buffer, 0, UART_INPUT_SIZE<<1);
	uart_data.rx_side   = 1;
	uart_data.rx_idx    = 0;
	uart_data.rx_size   = 0;
	uart_data.rx_flag   = 0;

	uart_data.tx_buffer = NULL;
	uart_data.tx_size   = 0;
	uart_data.tx_idx    = 0;
}

void uart_start(void)
{
	/* Enable Receiver Not Empty (RXNE) interrupt */
	ATOMIC_SET_BIT(huart2.Instance->CR1, USART_CR1_RXNEIE_RXFNEIE);

	/* Interrupts enable */
	HAL_NVIC_EnableIRQ(USART2_IRQn);
}

struct UART_Msg_Info uart_msg_pop(void)
{
	struct UART_Msg_Info ret = {
		.buffer = NULL,
		.size   = 0
	};

	if(uart_data.rx_flag) {
		ret.buffer        = &uart_data.rx_buffer[(1-uart_data.rx_side) << UART_INPUT_SIZE_POW];
		ret.size          = uart_data.rx_size;

		uart_data.rx_flag = 0; /* Acknowledge flag */
	}
	
	return ret;
}

uint8_t uart_transmit(const char *buffer, size_t size)
{
	if(uart_data.tx_size > 0) return 0; // Cannot transmit data, already busy

	uart_data.tx_size   = size;
	uart_data.tx_buffer = buffer;
	uart_data.tx_idx    = 0;

	/* Enable Transmitter empty (TXE) interrupt, transmit first char */
	uart_data.tx_expect_txe = 1; // Enabling TXEIE will trigger a TXE interrupt to start transfer
    ATOMIC_SET_BIT(huart2.Instance->CR1, USART_CR1_TXEIE_TXFNFIE);

	return 1;
}

uint8_t uart_transmit_done(void)
{
	/* Transmit is done if all characters have been consumed */
	return uart_data.tx_size == 0;
}

/* ┌────────────────────────────────────────┐
   │ Interrupt handler                      │
   └────────────────────────────────────────┘ */

void USART2_IRQHandler(void)
{
	uint32_t isrflags   = READ_REG(huart2.Instance->ISR);
	char     rd;


	/* ───────── RXNE: Read not empty ───────── */
	
	if(isrflags & UART_FLAG_RXNE) {
		rd = huart2.Instance->RDR; // Read character, clears RXNE flag

		switch(rd) {
			/* Ignored chars */
			case 0:
			case '\r':
			case '\n':
				break;

			/* Line break */
			//case '\n':
			//	uart_data.rx_buffer[(uart_data.rx_side<<UART_INPUT_SIZE_POW)+uart_data.rx_idx] = rd; // Include newline

			//	uart_data.rx_size   = uart_data.rx_idx+1;      // +1 to include newline
			//	uart_data.rx_idx    = 0;                       // Reset read index
			//	uart_data.rx_side   = 1 - uart_data.rx_side;   // Buffer swap
			//	uart_data.rx_flag   = 1;                       // A message is available!
			//	break;

			case CHAR_STX:
				// TODO
				break;

			case CHAR_ETX:
				uart_data.rx_size = uart_data.rx_idx;      // Do not include ETX
				uart_data.rx_idx  = 0;                     // Reset read index
				uart_data.rx_side = 1 - uart_data.rx_side; // Buffer swap
				uart_data.rx_flag = 1;                     // A message is available!
				break;

			/* Default: append to buffer and continue */
			default:
				/* Save character, increment read index */
				uart_data.rx_buffer[(uart_data.rx_side<<UART_INPUT_SIZE_POW)+uart_data.rx_idx] = rd;
				uart_data.rx_idx++;
				uart_data.rx_idx %= UART_INPUT_SIZE;
				break;
		}
	}


	/* ──────── TXE: Transmitter empty ──────── */

	else if(isrflags & UART_FLAG_TXE) {
		if(uart_data.tx_expect_txe) {
			uart_data.tx_expect_txe = 0;

			/* Transmit next char */
			uart_tx_next();

			/* Update counter stuff */
			uart_data.tx_size--;
			uart_data.tx_idx++;
			
			/* Transfer ended */
			if(!uart_data.tx_size) {
				uart_data.tx_buffer = NULL;

				/* Disable TXE interrupt */
				ATOMIC_CLEAR_BIT(huart2.Instance->CR1, USART_CR1_TXEIE_TXFNFIE);
			}
		}
	}
}
