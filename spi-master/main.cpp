#include "show-running.h"

#include <platform/mbed_assert.h>
#include <rtos/ThisThread.h>
#include <rtos/Thread.h>
#include <targets/TARGET_STM/TARGET_STM32F7/STM32Cube_FW/STM32F7xx_HAL_Driver/stm32f7xx_hal.h>
#include <targets/TARGET_STM/TARGET_STM32F7/STM32Cube_FW/STM32F7xx_HAL_Driver/stm32f7xx_ll_gpio.h>

#include <cstdio>

namespace
{

SPI_HandleTypeDef hspi = {
    .Instance = SPI1,
    .Init = {
        .Mode = SPI_MODE_MASTER,
        .Direction = SPI_DIRECTION_2LINES,
        .DataSize = SPI_DATASIZE_8BIT,
        .CLKPolarity = SPI_POLARITY_LOW,
        .CLKPhase = SPI_PHASE_1EDGE,
        .NSS = SPI_NSS_HARD_OUTPUT,
        // SPI1 uses APB2 clock. From system_clock.c "APB2CLK (MHz) | 108".
        // Hence SCLK = 108/16 = 6.75 MHz. I confirmed this using the scope.
        .BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16,
        .FirstBit = SPI_FIRSTBIT_MSB,
        .TIMode = SPI_TIMODE_DISABLE,
        .CRCCalculation = SPI_CRCCALCULATION_DISABLE,
        .CRCPolynomial = 1,
        .CRCLength = SPI_CRC_LENGTH_DATASIZE,
        .NSSPMode = SPI_NSS_PULSE_DISABLE
    },
    .pTxBuffPtr = nullptr,
    .TxXferSize = 0,
    .TxXferCount = 0,
    .pRxBuffPtr = nullptr,
    .RxXferSize = 0,
    .RxXferCount = 0,
    .CRCSize = 0,
    .RxISR = nullptr,
    .TxISR = nullptr,
    .hdmatx = nullptr,
    .hdmarx = nullptr,
    .Lock = HAL_UNLOCKED,
    .State = HAL_SPI_STATE_RESET,
    .ErrorCode = HAL_SPI_ERROR_NONE
};

uint8_t tx_buffer[] = { 's', 'p', 'i', ' ' };

const size_t stack_size = OS_STACK_SIZE; // /Normal/ stack size
MBED_ALIGN(8) unsigned char stack[stack_size];
rtos::Thread thread(osPriorityNormal, sizeof(stack), stack, "spi tx");


void spi_init() {
    MBED_UNUSED const auto status = HAL_SPI_Init(&hspi);
    MBED_ASSERT(status == HAL_OK);
}

void spi_tx() {
    using namespace std::chrono_literals;

    spi_init();

    while (1) {
        LL_GPIO_ResetOutputPin(GPIOD, LL_GPIO_PIN_14);

        MBED_UNUSED const auto status = HAL_SPI_Transmit(&hspi, &tx_buffer[0], sizeof(tx_buffer), 1 /*ms*/);
        MBED_ASSERT(status == HAL_OK);

        LL_GPIO_SetOutputPin(GPIOD, LL_GPIO_PIN_14);

        rtos::ThisThread::sleep_for(1ms);
    }
}

void spi_tx_init() {
    MBED_UNUSED const auto status = thread.start(spi_tx);
    MBED_ASSERT(status == osOK);
}

}

// Override /weak/ implementation provided by stm32f7xx_hal_spi.c.
// The clocks and IO could just be initialised before calling 'HAL_SPI_Init'
// but this method is a common theme in all HAL drivers so I'm going to
// give it a go here.
extern "C" void HAL_SPI_MspInit(SPI_HandleTypeDef *) {
    __HAL_RCC_SPI1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();

    // NSS PD14, CN7-16
    LL_GPIO_SetPinMode(GPIOD, LL_GPIO_PIN_14, LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetPinSpeed(GPIOD, LL_GPIO_PIN_14, LL_GPIO_SPEED_FREQ_VERY_HIGH);
    LL_GPIO_SetPinOutputType(GPIOD, LL_GPIO_PIN_14, LL_GPIO_OUTPUT_PUSHPULL);
    LL_GPIO_SetPinPull(GPIOD, LL_GPIO_PIN_14, LL_GPIO_PULL_NO);
    LL_GPIO_SetOutputPin(GPIOD, LL_GPIO_PIN_14);

    // SCLK PA5, CN7-10
    LL_GPIO_SetPinMode(GPIOA, LL_GPIO_PIN_5, LL_GPIO_MODE_ALTERNATE);
    LL_GPIO_SetAFPin_0_7(GPIOA, LL_GPIO_PIN_5, LL_GPIO_AF_5);
    LL_GPIO_SetPinSpeed(GPIOA, LL_GPIO_PIN_5, LL_GPIO_SPEED_FREQ_VERY_HIGH);
    LL_GPIO_SetPinOutputType(GPIOA, LL_GPIO_PIN_5, LL_GPIO_OUTPUT_PUSHPULL);
    LL_GPIO_SetPinPull(GPIOA, LL_GPIO_PIN_5, LL_GPIO_PULL_NO);

    // MISO PA6, CN7-12
    LL_GPIO_SetPinMode(GPIOA, LL_GPIO_PIN_6, LL_GPIO_MODE_ALTERNATE);
    LL_GPIO_SetAFPin_0_7(GPIOA, LL_GPIO_PIN_6, LL_GPIO_AF_5);
    LL_GPIO_SetPinSpeed(GPIOA, LL_GPIO_PIN_6, LL_GPIO_SPEED_FREQ_VERY_HIGH);
    LL_GPIO_SetPinPull(GPIOA, LL_GPIO_PIN_6, LL_GPIO_PULL_DOWN);

    // MOSI PA7, CN7-14
    LL_GPIO_SetPinMode(GPIOA, LL_GPIO_PIN_7, LL_GPIO_MODE_ALTERNATE);
    LL_GPIO_SetAFPin_0_7(GPIOA, LL_GPIO_PIN_7, LL_GPIO_AF_5);
    LL_GPIO_SetPinSpeed(GPIOA, LL_GPIO_PIN_7, LL_GPIO_SPEED_FREQ_VERY_HIGH);
    LL_GPIO_SetPinOutputType(GPIOA, LL_GPIO_PIN_7, LL_GPIO_OUTPUT_PUSHPULL);
    LL_GPIO_SetPinPull(GPIOA, LL_GPIO_PIN_7, LL_GPIO_PULL_NO);
}

int main() {
    puts("spi-master");

    show_running::init();

    spi_tx_init();

    rtos::ThisThread::sleep_for(rtos::Kernel::wait_for_u32_forever);
}
