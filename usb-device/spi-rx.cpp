#include "spi-rx.h"

#include <platform/mbed_assert.h>
#include <rtos/ThisThread.h>
#include <rtos/Thread.h>
#include <targets/TARGET_STM/TARGET_STM32F7/STM32Cube_FW/STM32F7xx_HAL_Driver/stm32f7xx_hal.h>
#include <targets/TARGET_STM/TARGET_STM32F7/STM32Cube_FW/STM32F7xx_HAL_Driver/stm32f7xx_ll_gpio.h>

namespace spi_rx
{

namespace
{

extern DMA_HandleTypeDef hdma;

SPI_HandleTypeDef hspi = {
    .Instance = SPI2,
    .Init = {
        .Mode = SPI_MODE_SLAVE,
        .Direction = SPI_DIRECTION_2LINES_RXONLY,  // Disable the output, MISO, although it probably doesn't make much difference.
        .DataSize = SPI_DATASIZE_8BIT,
        .CLKPolarity = SPI_POLARITY_LOW,
        .CLKPhase = SPI_PHASE_1EDGE,
        .NSS = SPI_NSS_HARD_INPUT,
        // Baud rate prescaler irrelevant for slave device
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
    .hdmarx = &hdma,
    .Lock = HAL_UNLOCKED,
    .State = HAL_SPI_STATE_RESET,
    .ErrorCode = HAL_SPI_ERROR_NONE
};

DMA_HandleTypeDef hdma = {
    .Instance = DMA1_Stream3,
    .Init = {
        .Channel = DMA_CHANNEL_0,
        .Direction = DMA_PERIPH_TO_MEMORY,
        .PeriphInc = DMA_PINC_DISABLE,
        .MemInc = DMA_MINC_ENABLE,
        .PeriphDataAlignment = DMA_PDATAALIGN_BYTE,
        .MemDataAlignment = DMA_MDATAALIGN_BYTE,
        // Circular transfer will keep going until it is stopped from software.
        .Mode = DMA_NORMAL,
        .Priority = DMA_PRIORITY_LOW,
        .FIFOMode = DMA_FIFOMODE_DISABLE,
        .FIFOThreshold = DMA_FIFO_THRESHOLD_FULL,
        .MemBurst = DMA_MBURST_INC4,
        .PeriphBurst = DMA_PBURST_INC4
    },
    .Lock = HAL_UNLOCKED,
    .State = HAL_DMA_STATE_RESET,
    .Parent = &hspi,
    .XferCpltCallback = nullptr,
    .XferHalfCpltCallback = nullptr,
    .XferM1CpltCallback = nullptr,
    .XferM1HalfCpltCallback = nullptr,
    .XferErrorCallback = nullptr,
    .XferAbortCallback = nullptr,
    .ErrorCode = HAL_DMA_ERROR_NONE,
    .StreamBaseAddress = 0,
    .StreamIndex = 0
};

const size_t stack_size = OS_STACK_SIZE; // /Normal/ stack size
MBED_ALIGN(8) unsigned char stack[stack_size];
rtos::Thread thread(osPriorityNormal, sizeof(stack), stack, "spi_rx");

const uint32_t rx_complete_flag = 1 << 0;

uint8_t rx_buffer[256] = { 0 };

void spi_init() {
    MBED_UNUSED const auto status = HAL_SPI_Init(&hspi);
    MBED_ASSERT(status == HAL_OK);
}

void dma_init() {
    __HAL_RCC_DMA1_CLK_ENABLE();

    MBED_UNUSED const auto status = HAL_DMA_Init(&hdma);
    MBED_ASSERT(status == HAL_OK);

    HAL_NVIC_SetPriority(DMA1_Stream3_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(DMA1_Stream3_IRQn);
}

// Use red LED to indicate receiving SPI data.
void led_init() {
    __HAL_RCC_GPIOA_CLK_ENABLE();

    // PA7, SYS_LD_USER1, red
    LL_GPIO_SetPinMode(GPIOA, LL_GPIO_PIN_7, LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetPinSpeed(GPIOA, LL_GPIO_PIN_7, LL_GPIO_SPEED_FREQ_LOW);
    LL_GPIO_SetPinOutputType(GPIOA, LL_GPIO_PIN_7, LL_GPIO_OUTPUT_PUSHPULL);
    LL_GPIO_SetPinPull(GPIOA, LL_GPIO_PIN_7, LL_GPIO_PULL_NO);
    LL_GPIO_ResetOutputPin(GPIOA, LL_GPIO_PIN_7);
}

void spi_rx() {
    using namespace std::chrono_literals;

    spi_init();
    dma_init();
    led_init();

    while (1) {
        MBED_UNUSED const auto status = HAL_SPI_Receive_DMA(&hspi, &rx_buffer[0], sizeof(rx_buffer));
        MBED_ASSERT(status == HAL_OK);

        MBED_UNUSED const auto result = rtos::ThisThread::flags_wait_all(rx_complete_flag);
        MBED_ASSERT(!(result & osFlagsError));

        LL_GPIO_TogglePin(GPIOA, LL_GPIO_PIN_7);

        rtos::ThisThread::sleep_for(500ms);
    }
}

}

void init() {
    MBED_UNUSED const auto status = thread.start(spi_rx);
    MBED_ASSERT(status == osOK);
}

}

// Override /weak/ implementation provided by stm32f7xx_hal_spi.c.
// The clocks and IO could just be initialised before calling 'HAL_SPI_Init'
// but this method is a common theme in all HAL drivers so I'm going to
// give it a go here.
extern "C" void HAL_SPI_MspInit(SPI_HandleTypeDef *) {
    __HAL_RCC_SPI2_CLK_ENABLE();
    __HAL_RCC_GPIOH_CLK_ENABLE();
    __HAL_RCC_GPIOI_CLK_ENABLE();

    // PH15, PMOD SEL 0, 0 == SPI, pulled up on board
    LL_GPIO_SetPinMode(GPIOH, LL_GPIO_PIN_15, LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetPinSpeed(GPIOH, LL_GPIO_PIN_15, LL_GPIO_SPEED_FREQ_LOW);
    LL_GPIO_SetPinOutputType(GPIOH, LL_GPIO_PIN_15, LL_GPIO_OUTPUT_PUSHPULL);
    LL_GPIO_SetPinPull(GPIOH, LL_GPIO_PIN_15, LL_GPIO_PULL_NO);
    LL_GPIO_ResetOutputPin(GPIOH, LL_GPIO_PIN_15);

    // PI10, PMOD SEL 1, 0 == SPI, no strictly necessary as pulled down on board
    LL_GPIO_SetPinMode(GPIOI, LL_GPIO_PIN_10, LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetPinSpeed(GPIOI, LL_GPIO_PIN_10, LL_GPIO_SPEED_FREQ_LOW);
    LL_GPIO_SetPinOutputType(GPIOI, LL_GPIO_PIN_10, LL_GPIO_OUTPUT_PUSHPULL);
    LL_GPIO_SetPinPull(GPIOI, LL_GPIO_PIN_10, LL_GPIO_PULL_NO);
    LL_GPIO_ResetOutputPin(GPIOI, LL_GPIO_PIN_10);

    // SCLK PI1, P2 PMOD#4
    LL_GPIO_SetPinMode(GPIOI, LL_GPIO_PIN_1, LL_GPIO_MODE_ALTERNATE);
    LL_GPIO_SetAFPin_0_7(GPIOI, LL_GPIO_PIN_1, LL_GPIO_AF_5);
    LL_GPIO_SetPinSpeed(GPIOI, LL_GPIO_PIN_1, LL_GPIO_SPEED_FREQ_VERY_HIGH);
    LL_GPIO_SetPinOutputType(GPIOI, LL_GPIO_PIN_1, LL_GPIO_OUTPUT_PUSHPULL);
    LL_GPIO_SetPinPull(GPIOI, LL_GPIO_PIN_1, LL_GPIO_PULL_NO);

    // MISO PI2, P2 PMOD#3
    LL_GPIO_SetPinMode(GPIOI, LL_GPIO_PIN_2, LL_GPIO_MODE_ALTERNATE);
    LL_GPIO_SetAFPin_0_7(GPIOI, LL_GPIO_PIN_2, LL_GPIO_AF_5);
    LL_GPIO_SetPinSpeed(GPIOI, LL_GPIO_PIN_2, LL_GPIO_SPEED_FREQ_VERY_HIGH);
    LL_GPIO_SetPinPull(GPIOI, LL_GPIO_PIN_2, LL_GPIO_PULL_NO);

    // MOSI PI3, P2 PMOD#2
    LL_GPIO_SetPinMode(GPIOI, LL_GPIO_PIN_3, LL_GPIO_MODE_ALTERNATE);
    LL_GPIO_SetAFPin_0_7(GPIOI, LL_GPIO_PIN_3, LL_GPIO_AF_5);
    LL_GPIO_SetPinSpeed(GPIOI, LL_GPIO_PIN_3, LL_GPIO_SPEED_FREQ_VERY_HIGH);
    LL_GPIO_SetPinOutputType(GPIOI, LL_GPIO_PIN_3, LL_GPIO_OUTPUT_PUSHPULL);
    LL_GPIO_SetPinPull(GPIOI, LL_GPIO_PIN_3, LL_GPIO_PULL_NO);
}

// Override /weak/ implementation provided by stm32f7xx_hal_spi.c.
extern "C" void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi) {
    MBED_UNUSED const auto result = spi_rx::thread.flags_set(spi_rx::rx_complete_flag);
    MBED_ASSERT(!(result & osFlagsError));
}

// Override /weak/ implementation provided by startup_stm32f723xx.s.
extern "C" void DMA1_Stream3_IRQHandler() {
    HAL_DMA_IRQHandler(&spi_rx::hdma);
}
