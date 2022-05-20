/* ┌─────────────────────┐
   │ Simple GPIO control │
   └─────────────────────┘
   
    Florian Dupeyron
    May 2022
*/

#include "gpio.h"

void gpio_pin_init(struct Pin_Def pin, uint32_t mode, uint32_t pull, uint32_t speed, uint32_t alternate)
{
	GPIO_InitTypeDef settings = {
		.Pin       = pin.pin,
		.Mode      = mode,
		.Pull      = pull,
		.Speed     = speed,
		.Alternate = alternate
	};

	/* Switch clock ON */
	if     (pin.port == GPIOA) __HAL_RCC_GPIOA_CLK_ENABLE();
	else if(pin.port == GPIOB) __HAL_RCC_GPIOB_CLK_ENABLE();
	else if(pin.port == GPIOC) __HAL_RCC_GPIOC_CLK_ENABLE();
	else if(pin.port == GPIOD) __HAL_RCC_GPIOD_CLK_ENABLE();
	else if(pin.port == GPIOF) __HAL_RCC_GPIOF_CLK_ENABLE();

	/* Init GPIO stuff */
	HAL_GPIO_Init(pin.port, &settings);
}

void gpio_pin_write(struct Pin_Def pin, uint8_t value)
{
	HAL_GPIO_WritePin(pin.port, pin.pin, value);
}

uint8_t gpio_pin_read(struct Pin_Def pin)
{
	return HAL_GPIO_ReadPin(pin.port, pin.pin);
}
