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
#include <io/dmx.h>


UART_HandleTypeDef huart2;
struct DMX_Controller dmx_controller = {
	.uart        = USART1,
	.pin_output  = &pin_dmx_out,
	.pin_uart_af = GPIO_AF1_USART1
};

static void MX_USART2_UART_Init(void);

int main(void)
{
	HAL_Init();
	clock_init();
	
	MX_USART2_UART_Init();

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

	/* DMX init */
	
	dmx_controller_init (&dmx_controller);

	/* Let's go! */
	
	//dmx_controller_start(&dmx_controller);

	while(1) {
		gpio_pin_write(pin_led, 1);
		HAL_Delay(250);
		gpio_pin_write(pin_led, 0);
		HAL_Delay(250);
	};
}

static void MX_USART2_UART_Init(void)
{
	huart2.Instance = USART2;
	huart2.Init.BaudRate = 115200;
	huart2.Init.WordLength = UART_WORDLENGTH_7B;
	huart2.Init.StopBits = UART_STOPBITS_1;
	huart2.Init.Parity = UART_PARITY_NONE;
	huart2.Init.Mode = UART_MODE_TX_RX;
	huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	huart2.Init.OverSampling = UART_OVERSAMPLING_16;
	huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
	huart2.Init.ClockPrescaler = UART_PRESCALER_DIV1;
	huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
	if (HAL_UART_Init(&huart2) != HAL_OK)
	{
		Error_Handler();
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
