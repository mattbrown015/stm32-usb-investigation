#include "show-running.h"

#include "main.h"

#include <targets/TARGET_STM/TARGET_STM32F7/STM32Cube_FW/STM32F7xx_HAL_Driver/stm32f7xx_ll_gpio.h>

namespace show_running
{

namespace
{

// Use green LED to indicate thread running.
void led_init() {
    __HAL_RCC_GPIOB_CLK_ENABLE();

    // User LD1 green
    LL_GPIO_SetPinMode(GPIOB, LL_GPIO_PIN_0, LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetPinSpeed(GPIOB, LL_GPIO_PIN_0, LL_GPIO_SPEED_FREQ_LOW);
    LL_GPIO_SetPinOutputType(GPIOB, LL_GPIO_PIN_0, LL_GPIO_OUTPUT_PUSHPULL);
    LL_GPIO_SetPinPull(GPIOB, LL_GPIO_PIN_0, LL_GPIO_PULL_NO);
    LL_GPIO_ResetOutputPin(GPIOB, LL_GPIO_PIN_0);
}

void show_running() {
    LL_GPIO_TogglePin(GPIOB, LL_GPIO_PIN_0);
}

}

void init() {
    using namespace std::chrono_literals;

    led_init();

    event_queue.call_every(1s, show_running);
}

}
