/* ┌────────────────────────────────────────┐
   │ Simple software timer using a timebase │
   └────────────────────────────────────────┘
   
    Florian Dupeyron
    May 2022
*/

#include "oneshot_timer.h"
#include "main.h"

#include <bsp/pin.h>
#include <io/gpio.h>


/* ┌────────────────────────────────────────┐
   │ Static private data                    │
   └────────────────────────────────────────┘ */

struct OneShot_Timer_Private {
	TIM_HandleTypeDef htim;

	Oneshot_Timer_Callback done_cbk;
	void *usrdata;

	volatile uint8_t tick;
};

static struct OneShot_Timer_Private __stimer_private;

/* ┌────────────────────────────────────────┐
   │ Private interface                      │
   └────────────────────────────────────────┘ */

void __oneshot_timer_hal_init(struct OneShot_Timer_Private *stim)
{
	ONESHOT_TIMER_CLK_ENABLE();

	TIM_ClockConfigTypeDef  sClockSourceConfig = {0};
	TIM_MasterConfigTypeDef sMasterConfig      = {0};
	uint32_t                clkFreq            = HAL_RCC_GetHCLKFreq();

	uint32_t prescaler;
	uint32_t period;

	/* Init timer settings */
	
	stim->htim.Instance               = ONESHOT_TIMER_INSTANCE;
	stim->htim.Init.CounterMode       = TIM_COUNTERMODE_UP;
	stim->htim.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
	stim->htim.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

	/* Configure timer for 1us ticks */
	stim->htim.Init.Prescaler         = 16;
	stim->htim.Init.Period            = 60000; // 60ms

	if(HAL_TIM_Base_Init(&stim->htim) != HAL_OK) {
		Error_Handler();
	}


	/* Clock source config */
	sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
	if(HAL_TIM_ConfigClockSource(&stim->htim, &sClockSourceConfig) != HAL_OK) {
		Error_Handler();
	}


	/* Master output trigger */
	sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
	sMasterConfig.MasterSlaveMode     = TIM_MASTERSLAVEMODE_DISABLE;
	if(HAL_TIMEx_MasterConfigSynchronization(&stim->htim, &sMasterConfig) != HAL_OK) {
		Error_Handler();
	}
}


void __oneshot_timer_irq_config(struct OneShot_Timer_Private *stim)
{
	HAL_NVIC_SetPriority(ONESHOT_TIMER_IRQ, 3, 0);
	HAL_NVIC_EnableIRQ  (ONESHOT_TIMER_IRQ);
}


void __oneshot_timer_done(struct OneShot_Timer_Private *stimer)
{
	HAL_TIM_Base_Stop_IT(&stimer->htim);

	if(stimer->done_cbk != NULL) stimer->done_cbk(stimer->usrdata);
}


/* ┌────────────────────────────────────────┐
   │ Public interface                       │
   └────────────────────────────────────────┘ */

void oneshot_timer_init(Oneshot_Timer_Callback done_cbk, void *usrdata)
{
	__oneshot_timer_hal_init    (&__stimer_private);
	__oneshot_timer_irq_config  (&__stimer_private);

	__stimer_private.done_cbk = done_cbk;
	__stimer_private.usrdata  = usrdata;
}


void oneshot_timer_start(uint32_t delay_ms)
{
	/* Set timer period */
	__stimer_private.htim.Init.Period = delay_ms;
	if(HAL_TIM_Base_Init(&__stimer_private.htim) != HAL_OK) {
		Error_Handler();
	}

	/* Start timer */
	__stimer_private.tick = 0;
	if(HAL_TIM_Base_Start_IT(&__stimer_private.htim) != HAL_OK) {
		Error_Handler();
	}

}


/* ┌────────────────────────────────────────┐
   │ IRQs                                   │
   └────────────────────────────────────────┘ */

void ONESHOT_TIMER_ISR(void)
{
	if(__HAL_TIM_GET_FLAG(&__stimer_private.htim, TIM_FLAG_UPDATE)) {
		__stimer_private.tick++;
		if(__stimer_private.tick >= 2) {
			__oneshot_timer_done(&__stimer_private);
		}
	}


	/* Clears interrupts and does HAL stuff */

	HAL_TIM_IRQHandler(&__stimer_private.htim);
}
