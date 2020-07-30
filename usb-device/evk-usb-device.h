#ifndef EVKUSBDEVICE_H
#define EVKUSBDEVICE_H

#if defined(DEVICE_USBDEVICE)

#include <drivers/internal/USBDevice.h>
#include <targets/TARGET_STM/TARGET_STM32F7/STM32Cube_FW/CMSIS/stm32f7xx.h>
#include <targets/TARGET_STM/TARGET_STM32F7/STM32Cube_FW/STM32F7xx_HAL_Driver/stm32f7xx_ll_usb.h>

namespace EvkUSBDevice
{

class EvkUSBDevice : public USBDevice {
public:
    EvkUSBDevice();
    ~EvkUSBDevice() override;

    void wait_configured();

    uint32_t bulk_in_transfer(uint8_t *buffer, uint32_t size);
    uint32_t bulk_out_transfer(uint8_t *buffer, uint32_t size);

private:
    const uint8_t *configuration_desc(uint8_t index) override;

    void callback_state_change(DeviceState new_state) override;
    void callback_request(const setup_packet_t *setup) override;
    void callback_request_xfer_done(const setup_packet_t *setup, bool aborted) override;
    void callback_set_configuration(uint8_t configuration) override;
    void callback_set_interface(uint16_t interface, uint8_t alternate) override;

private:
    // The USB standard states that the maximum packet size for ctrl endpoints, like endpoint 0, is 64 bytes.
    static const auto ep0_maximum_packet_size = USB_OTG_MAX_EP0_SIZE;
    // The maximum packet size for bulk endpoints depends on the USB speed.
    static const auto bulk_ep_maximum_packet_size =
#if (MBED_CONF_TARGET_USB_SPEED == USE_USB_OTG_HS)
        // The maximum packet size should be 512 but when it is 512
        // the assert at USBPhy_STM32.cpp:471 fires.
        // I guess this is related to 'tx_ep_sizes' in USBPhy_STM32.cpp.
        USB_OTG_HS_MAX_PACKET_SIZE
#elif (MBED_CONF_TARGET_USB_SPEED == USE_USB_OTG_FS)
        USB_OTG_FS_MAX_PACKET_SIZE
#else
# error unexpected USB speed
#endif
        ;


    static const uint8_t default_configuration = 1;
    static const size_t configuration_descriptor_length = 32;
    uint8_t configuration_descriptor[configuration_descriptor_length];

    usb_ep_t epbulk_in;
    usb_ep_t epbulk_out;
    uint8_t epbulk_out_buffer[bulk_ep_maximum_packet_size];
    uint32_t epbulk_out_buffer_bytes_available;

    void epbulk_in_callback();
    void epbulk_out_callback();
};

}

#endif

#endif
