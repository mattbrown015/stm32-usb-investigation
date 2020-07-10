#ifndef EVKUSBDEVICE_H
#define EVKUSBDEVICE_H

#include <drivers/internal/USBDevice.h>

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
    // The derivation of the maximum packet size is not yet clear to me.
    // 64 appears a great deal in the examples so it is what I'm using for now.
    static const auto maximum_packet_size = 64;

    static const uint8_t default_configuration = 1;
    static const size_t configuration_descriptor_length = 32;
    uint8_t configuration_descriptor[configuration_descriptor_length];

    usb_ep_t epbulk_in;
    usb_ep_t epbulk_out;
    uint8_t epbulk_out_buffer[maximum_packet_size];
    uint32_t epbulk_out_buffer_bytes_available;

    void epbulk_in_callback();
    void epbulk_out_callback();
};

}

#endif
