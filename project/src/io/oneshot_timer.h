/* ┌───────────────────────────────────────────────────┐
   │ Simple oneshot timer to wait for specific actions │
   └───────────────────────────────────────────────────┘
   
    Florian Dupeyron
    May 2022
*/



#pragma once

#include <inttypes.h>


/* ┌────────────────────────────────────────┐
   │ Oneshot timer config                   │
   └────────────────────────────────────────┘ */

#define ONESHOT_TIMER_INSTANCE     TIM17
#define ONESHOT_TIMER_IRQ          TIM17_IRQn
#define ONESHOT_TIMER_ISR          TIM17_IRQHandler
#define ONESHOT_TIMER_CLK_ENABLE __HAL_RCC_TIM17_CLK_ENABLE

typedef void (*Oneshot_Timer_Callback)(void);


/* ┌────────────────────────────────────────┐
   │ Public interface                       │
   └────────────────────────────────────────┘ */

void    oneshot_timer_init (Oneshot_Timer_Callback done_cbk);
void    oneshot_timer_start(uint32_t delay_us);
