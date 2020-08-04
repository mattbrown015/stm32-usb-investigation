#include "show-running.h"

#include <rtos/ThisThread.h>
#include <rtos/Thread.h>
#include <targets/TARGET_STM/TARGET_STM32F7/STM32Cube_FW/STM32F7xx_HAL_Driver/stm32f7xx_ll_gpio.h>

namespace show_running
{

namespace
{

const size_t stack_size = 512; // Arbitrarily /small/ stack
MBED_ALIGN(8) unsigned char stack[stack_size];
rtos::Thread thread(osPriorityLow, sizeof(stack), stack, "show_running");

// Use green LED to indicate thread running.
void led_init() {
    __HAL_RCC_GPIOB_CLK_ENABLE();

    // PB1, SYS_LD_USER2, green
    LL_GPIO_SetPinMode(GPIOB, LL_GPIO_PIN_1, LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetPinSpeed(GPIOB, LL_GPIO_PIN_1, LL_GPIO_SPEED_FREQ_LOW);
    LL_GPIO_SetPinOutputType(GPIOB, LL_GPIO_PIN_1, LL_GPIO_OUTPUT_PUSHPULL);
    LL_GPIO_SetPinPull(GPIOB, LL_GPIO_PIN_1, LL_GPIO_PULL_NO);
    LL_GPIO_ResetOutputPin(GPIOB, LL_GPIO_PIN_1);
}


void show_running() {
    using namespace std::chrono_literals;

    led_init();

    while (1) {
        LL_GPIO_TogglePin(GPIOB, LL_GPIO_PIN_1);

        rtos::ThisThread::sleep_for(1s);
    }
}

}

void init() {
    MBED_UNUSED const auto status = thread.start(show_running);
    MBED_ASSERT(status == osOK);
}

}
