#include "evk-usb-device-hal.h"

#include <platform/mbed_assert.h>
#include <targets/TARGET_STM/TARGET_STM32F7/STM32Cube_FW/STM32F7xx_HAL_Driver/stm32f7xx_hal.h>

namespace evk_usb_device_hal
{

PCD_HandleTypeDef hpcd = {
    .Instance = USB_OTG_HS,
    .Init = {
        .dev_endpoints = 9,  // STM32F72x has 1 bidirectional control endpoint0 and 8 HS endpoints configurable to support bulk, interrupt or isochronous transfers
        .Host_channels = 0,  // STM32F72x has 16 HS host channels but setting to 0 as not using host mode
        .speed = PCD_SPEED_HIGH,
        // Disable embedded DMA initially although need to find out how to use DMA.
        // HAL_PCD_Init sets this to DISABLE depending on the value of OTG_CID i.e. it checks if this is a HS device
        .dma_enable = DISABLE,
        .ep0_mps = USB_OTG_MAX_EP0_SIZE,
        .phy_itface = USB_OTG_HS_EMBEDDED_PHY,
        .Sof_enable = DISABLE,
        .low_power_enable = DISABLE,
        .lpm_enable = DISABLE,
        .battery_charging_enable = DISABLE,
        .vbus_sensing_enable = ENABLE,
        .use_dedicated_ep1 = DISABLE,
        .use_external_vbus = DISABLE
    },
    .USB_Address = 0,  // HAL_PCD_Init set this to 0
    // HAL_PCD_Init initialises the IN_ep and OUT_ep upto the number specified in in dev_endpoints.
    .IN_ep = { 0 },
    .OUT_ep = { 0 },
    .Lock = HAL_UNLOCKED,
    .State = HAL_PCD_STATE_RESET,
    .ErrorCode = 0,
    .Setup = { 0 },
    .LPM_State = LPM_L0,
    .BESL = 0,
    .lpm_active = DISABLE,
    .pData = nullptr,
};

void init() {
    __HAL_RCC_OTGPHYC_CLK_ENABLE();
    __HAL_RCC_USB_OTG_HS_CLK_ENABLE();
    __HAL_RCC_USB_OTG_HS_ULPI_CLK_ENABLE();

    MBED_UNUSED const HAL_StatusTypeDef ret = HAL_PCD_Init(&hpcd);
    MBED_ASSERT(ret == HAL_OK);
}

}
