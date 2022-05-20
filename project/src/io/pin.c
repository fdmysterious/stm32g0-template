/* ┌─────────────────┐
   │ Pins definition │
   └─────────────────┘
   
    Florian Dupeyron
    May 2022
*/

#include "pin.h"

/* ┌────────────────────────────────────────┐
   │ Pin list                               │
   └────────────────────────────────────────┘ */

const struct Pin_Def pin_led    = { .port = GPIOC, .pin = GPIO_PIN_6 };

const struct Pin_Def pin_vcp_rx = { .port = GPIOA, .pin = GPIO_PIN_3 };
const struct Pin_Def pin_vcp_tx = { .port = GPIOA, .pin = GPIO_PIN_2 };

const struct Pin_Def pin_nrst   = { .port = GPIOF, .pin = GPIO_PIN_2 };
