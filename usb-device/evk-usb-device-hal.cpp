#include "evk-usb-device-hal.h"

#include <platform/mbed_assert.h>
#include <targets/TARGET_STM/TARGET_STM32F7/STM32Cube_FW/STM32F7xx_HAL_Driver/stm32f7xx_hal.h>

#include <type_traits>

namespace evk_usb_device_hal
{

namespace
{

// From Table 9-2. Format of Setup Data...
enum class recipient: uint8_t {
    device = 0x00,
    interface = 0x01,
    endpoint = 0x02,
    other = 0x03,
    mask = 0x1f
};

enum class type: uint8_t {
    standard = 0x00,
    class_ = 0x20,
    vendor = 0x40,
    mask = 0x60
};

enum class direction: uint8_t {
    host_to_device = 0x00,
    device_to_host = 0x80,
    mask = 0x80
};

// From Table 9-4. Standard Request Codes...
enum class request: uint8_t {
    get_status = 0,
    clear_feature = 1,
    set_feature = 3,
    set_address = 5,
    get_descriptor = 6,
    set_descriptor = 7,
    get_configuration = 8,
    set_configuration = 9,
    get_interface = 10,
    set_interface = 11,
    synch_frame = 12
};

// From Table 9-5. Descriptor Types...
enum class descriptor: uint8_t {
    device = 1,
    configuration = 2,
    string = 3,
    interface = 4,
    endpoint = 5,
    device_qualifier = 6,
    other_speed_configuration = 7,
    interface_power = 8
};

const size_t device_descriptor_length = 18;
uint8_t device_descriptor[device_descriptor_length] = {
    // device descriptor, USB spec 9.6.1
    device_descriptor_length,  // bLength
    1,                      // bDescriptorType
    0x0200 & 0xff,          // bcdUSB
    (0x0200 & 0xff00) >> 8,
    0x00,                   // bDeviceClass
    0x00,                   // bDeviceSubClass
    0x00,                   // bDeviceprotocol
    USB_OTG_MAX_EP0_SIZE,   // bMaxPacketSize0
    0x1f00 & 0xff,          // idVendor
    (0x1f00 & 0xff00) >> 8,
    0x2012 & 0xff,          // idProduct
    (0x2012 & 0xff00) >> 8,
    0x0001 & 0xff,          // bcdDevice
    (0x0001 & 0xff00) >> 8,
    0,                      // iManufacturer
    0,                      // iProduct
    0,                      // iSerialNumber
    0                       // bNumConfigurations
};

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

// https://stackoverflow.com/a/33083231
template <typename E>
constexpr auto to_underlying(E e) noexcept {
    return static_cast<std::underlying_type_t<E>>(e);
}

void setup_stage_callback(PCD_HandleTypeDef *const hpcd) {
    const uint8_t bmRequestType = hpcd->Setup[0] & 0xff;
    const uint8_t recipient = bmRequestType & to_underlying(recipient::mask);
    const uint8_t type = bmRequestType & to_underlying(type::mask);
    const uint8_t direction = bmRequestType & to_underlying(direction::mask);

    const uint8_t bRequest = (hpcd->Setup[0] & 0xff00) >> 8;
    const uint16_t wValue = (hpcd->Setup[0] & 0xffff0000) >> 16;
    const uint8_t descriptor_type = (wValue & 0xff00) >> 8;
    // const uint8_t descriptor_index = wValue & 0xff;

    if (recipient == to_underlying(recipient::device) && type == to_underlying(type::standard) && direction == to_underlying(direction::device_to_host)) {
        if (bRequest == to_underlying(request::get_descriptor)) {
            if (descriptor_type == to_underlying(descriptor::device)) {
                HAL_PCD_EP_Transmit(&evk_usb_device_hal::hpcd, 0, &evk_usb_device_hal::device_descriptor[0], evk_usb_device_hal::device_descriptor_length);
            }
        }
    }
}

}

void init() {
    // 'STM32Cube_FW_F7_V1.16.0/Projects/STM32F723E-Discovery/Applications/USB_Device/HID_Standalone/Src/usbd_conf.c'
    // uses 'HAL_PCD_MspInit' to do some of the configuration. 'HAL_PCD_MspInit' is called from 'HAL_PCD_Init' and it is
    // intended that the application will use it for low level configuration such as clocks and GPIOs.
    // I'm not sure what value it adds and 'USBPhyHw::init' doesn't use.
    // I'm just going to do all the initialisation here, e.g. the clocks are enabled below.
    // 'HAL_PCD_MspInit' in 'usbd_conf.c' initialises the GPIOs for USB FS but not for USB HS.
    // It appears the USB HS GPIOs don't need to be configured for their alternate functions.
    // I don't understand this and perhaps it will need revisiting.

    __HAL_RCC_OTGPHYC_CLK_ENABLE();
    __HAL_RCC_USB_OTG_HS_CLK_ENABLE();
    __HAL_RCC_USB_OTG_HS_ULPI_CLK_ENABLE();

    MBED_UNUSED const HAL_StatusTypeDef ret = HAL_PCD_Init(&hpcd);
    MBED_ASSERT(ret == HAL_OK);

    // The STM32F72x has 4 Kbytes of dedicated RAM for the USB data FIFOs in HS mode.
    // FIFO size is set as 32-bit words i.e. 0x200 words is 2048 bytes or half the dedicated RAM.
    // From 32.11.3 FIFO RAM allocation/Device Mode of the reference manual:
    //     10 locations must be reserved in the receive FIFO to receive SETUP packets on control endpoint.
    //     One location is to be allocated for Global OUT NAK.
    // By location I think it means 'word'.
    // The reference manual doesn't say where these reserved locations are but it appears from
    // 'USBPhyHw::init' and 'STM32Cube_FW_F7_V1.16.0/Projects/STM32F723E-Discovery/Applications/USB_Device/HID_Standalone/Src/usbd_conf.c'
    // that not all the words should be allocated here.
    // 'USBPhyHw::init' is a poor example because it thinks there is only '1.25 kbytes' of RAM available.
    // I've copied these numbers from 'usbd_conf.c'.
    // I think the rational is something like:
    //     Packets of varying sizes can be received so allocate an arbitrarily large RX FIFO i.e. half the available space.
    //     If the application will result in more TX packets then, I guess, this could be reduced.
    //     Control packets on EP0 have a max size of 64 bytes so I think 0x80 means there's room for 8 TX control packets in the FIFO.
    //     TX packets on EP1 could be 512 bytes so 0x174 words (1488 bytes) means there's room for nearly 3 packets in the FIFO.
    //     If the application will be sending  many bulk 512 byte packets I guess the TX FIFO size could be increased to make room for 3, or 4, packets.
    // 'usbd_conf.c' seems to leave 12 words unallocated when I expected 11. Either there's something I don't understand or there's another word up for grabs.
    // Stick with 'usbd_conf.c' until the example is working.
    HAL_PCDEx_SetRxFiFo(&hpcd, 0x200);  // Rx FIFO size must be set first
    HAL_PCDEx_SetTxFiFo(&hpcd, 0, 0x80);  // Tx FIFOs for IN endpoints must be set in order
    HAL_PCDEx_SetTxFiFo(&hpcd, 1, 0x174);

    // Set USB HS interrupt to the lowest priority
    HAL_NVIC_SetPriority(OTG_HS_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(OTG_HS_IRQn);

    HAL_PCD_Start(&hpcd);
}

}

// Replace /weak/ definition provided by 'startup_stm32f723xx.s' so needs to be in the global namespace.
extern "C" void OTG_HS_IRQHandler() {
    HAL_PCD_IRQHandler(&evk_usb_device_hal::hpcd);
}

// This is the first callback called after calling 'HAL_PCD_Start' and connecting the device.
// This is a significantly simplified version of 'HAL_PCD_ResetCallback' from 'usbd_conf.c'.
// A USB device must always have EP0 open for In and OUT transactions.
void HAL_PCD_ResetCallback(PCD_HandleTypeDef *const hpcd) {
    const uint8_t ep0_out_ep_addr = 0x00;
    HAL_PCD_EP_Open(hpcd, ep0_out_ep_addr, USB_OTG_MAX_EP0_SIZE, EP_TYPE_CTRL);

    const uint8_t ep0_in_ep_addr = 0x80;
    HAL_PCD_EP_Open(hpcd, ep0_in_ep_addr, USB_OTG_MAX_EP0_SIZE, EP_TYPE_CTRL);
}

// 'HAL_PCD_IRQHandler' decodes setup packets, puts the payload in 'hpcd->Setup'
// and calls 'HAL_PCD_SetupStageCallback'.
// The packet is described in the USB Spec 9.3 USB Device Requests.
void HAL_PCD_SetupStageCallback(PCD_HandleTypeDef *hpcd)
{
    evk_usb_device_hal::setup_stage_callback(hpcd);
}
