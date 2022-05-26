/* ┌────────────────────────────────────┐ │ Simple serial based DMX controller │
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
struct DMX_Controller dmx;
volatile uint8_t done;

static void MX_USART2_UART_Init(void);

void oneshot_done_callback(void)
{
	static uint8_t state;

	done = 1;

	state = 1 - state;
	gpio_pin_write(pin_led, state);
}

int main(void)
{
	HAL_Init();
	clock_init();
	oneshot_timer_init(oneshot_done_callback);
	
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
	
	dmx_controller_init(&dmx);

	while (1)
	{
		done = 0;
		oneshot_timer_start(60000);
		while(!done);
	}
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
