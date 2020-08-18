#include "main.h"

#include "show-running.h"

#include <platform/mbed_assert.h>
#include <targets/TARGET_STM/TARGET_STM32F7/STM32Cube_FW/STM32F7xx_HAL_Driver/stm32f7xx_hal.h>
#include <targets/TARGET_STM/TARGET_STM32F7/STM32Cube_FW/STM32F7xx_HAL_Driver/stm32f7xx_ll_exti.h>
#include <targets/TARGET_STM/TARGET_STM32F7/STM32Cube_FW/STM32F7xx_HAL_Driver/stm32f7xx_ll_gpio.h>
#include <targets/TARGET_STM/TARGET_STM32F7/STM32Cube_FW/STM32F7xx_HAL_Driver/stm32f7xx_ll_system.h>

#include <cstdio>

MBED_ALIGN(4) unsigned char event_queue_buffer[EVENTS_QUEUE_SIZE];
events::EventQueue event_queue(EVENTS_QUEUE_SIZE, &event_queue_buffer[0]);

namespace
{

extern DMA_HandleTypeDef hdma;

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
    .hdmatx = &hdma,
    .hdmarx = nullptr,
    .Lock = HAL_UNLOCKED,
    .State = HAL_SPI_STATE_RESET,
    .ErrorCode = HAL_SPI_ERROR_NONE
};

DMA_HandleTypeDef hdma = {
    .Instance = DMA2_Stream3,
    .Init = {
        .Channel = DMA_CHANNEL_3,
        .Direction = DMA_MEMORY_TO_PERIPH,
        .PeriphInc = DMA_PINC_DISABLE,
        .MemInc = DMA_MINC_ENABLE,
        // In direct mode PSIZE and MSIZE should be the same.
        // From the reference manual:
        //     In direct mode (DMDIS = 0 in the DMA_SxFCR register), the packing/unpacking of data is
        //     not possible. In this case, it is not allowed to have different source and destination transfer
        //     data widths: both are equal and defined by the PSIZE bits in the DMA_SxCR register.
        //     MSIZE bits are not relevant.
        // Which made me think MemDataAlignment was irrelevant in direct mode.
        // But to complicate things HAL_SPI_Transmit_DMA checks MemDataAlignment to
        // decide whether or not to enable packing.
        //       /* Packing mode is enabled only if the DMA setting is HALWORD */
        // I believe setting PSIZE and MSIZE to 'half-word' will reduce the number of DMA
        // transfers and improve utilisation but I can't prove it.
        // And, it probably doesn't matter when nothing else is using the bus.
        .PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD,
        .MemDataAlignment = DMA_MDATAALIGN_HALFWORD,
        // Circular transfer will keep going until it is stopped from software.
        .Mode = DMA_CIRCULAR,
        .Priority = DMA_PRIORITY_LOW,
        .FIFOMode = DMA_FIFOMODE_DISABLE,
        .FIFOThreshold = DMA_FIFO_THRESHOLD_FULL,
        // MemBurst and PeriphBurst are ignored when the FIFO is disabled.
        // From reference manual:
        //     In direct mode, the stream can only generate single transfers and the MBURST[1:0] and PBURST[1:0] bits are forced by hardware.
        .MemBurst = DMA_MBURST_SINGLE,
        .PeriphBurst = DMA_PBURST_SINGLE
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

uint8_t tx_buffer[] = { 's', 'p', 'i', ' ' };

bool dma_running = false;

// Use blue user button to start and stop SPI.
void button_init() {
    __HAL_RCC_SYSCFG_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    // PC13, USER (blue), pulled down on the board
    LL_GPIO_SetPinMode(GPIOC, LL_GPIO_PIN_13, LL_GPIO_MODE_INPUT);

    LL_SYSCFG_SetEXTISource(LL_SYSCFG_EXTI_PORTC, LL_SYSCFG_EXTI_LINE13);

    LL_EXTI_EnableIT_0_31(LL_EXTI_LINE_13);
    LL_EXTI_EnableFallingTrig_0_31(LL_EXTI_LINE_13);
    LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_13);

    HAL_NVIC_SetPriority(EXTI15_10_IRQn, 15, 15);
    HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
}

void spi_init() {
    MBED_UNUSED const auto status = HAL_SPI_Init(&hspi);
    MBED_ASSERT(status == HAL_OK);
}

void dma_init() {
    __HAL_RCC_DMA2_CLK_ENABLE();

    MBED_UNUSED const auto status = HAL_DMA_Init(&hdma);
    MBED_ASSERT(status == HAL_OK);

    HAL_NVIC_SetPriority(DMA2_Stream3_IRQn, 1, 1);
    HAL_NVIC_EnableIRQ(DMA2_Stream3_IRQn);
}

void start_circular_dma() {
    MBED_UNUSED const auto status = HAL_SPI_Transmit_DMA(&hspi, &tx_buffer[0], sizeof(tx_buffer));
    MBED_ASSERT(status == HAL_OK);
}

void stop_circular_dma() {
    MBED_UNUSED const auto status = HAL_SPI_DMAStop(&hspi);
    MBED_ASSERT(status == HAL_OK);
}

void toggle_dma() {
    if (!dma_running) {
        LL_GPIO_ResetOutputPin(GPIOD, LL_GPIO_PIN_14);

        start_circular_dma();

        dma_running = true;
    } else {
        stop_circular_dma();

        LL_GPIO_SetOutputPin(GPIOD, LL_GPIO_PIN_14);

        dma_running = false;
    }
}

void spi_tx_init() {
    button_init();
    spi_init();
    dma_init();

    toggle_dma();
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

// Override /weak/ implementation provided by startup_stm32f767xx.S.
extern "C" void DMA2_Stream3_IRQHandler() {
    HAL_DMA_IRQHandler(&hdma);
}

// Override /weak/ implementation provided by startup_stm32f767xx.S.
extern "C" void EXTI15_10_IRQHandler() {
    if (LL_EXTI_IsActiveFlag_0_31(LL_EXTI_LINE_13)) {
        event_queue.call(toggle_dma);

        LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_13);
    }
}

int main() {
    puts("spi-master");

    show_running::init();

    spi_tx_init();

    event_queue.dispatch_forever();
}
