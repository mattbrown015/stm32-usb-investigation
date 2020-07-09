#include <drivers/internal/USBDescriptor.h>
#include <drivers/internal/USBDevice.h>
#include <hal/usb/usb_phy_api.h>
#include <rtos/ThisThread.h>

#include <algorithm>
#include <cstdio>

// Global symbol easy to examine with debugger.
// Arbitrary length, just needs to longer than what's sent with the request.
uint8_t received_request_data[16] = { 0 };

namespace
{

class MyUSBDevice : public USBDevice {
public:
    MyUSBDevice();
    ~MyUSBDevice() override;

protected:
    const uint8_t *configuration_desc(uint8_t index) override;

    void callback_state_change(DeviceState new_state) override;
    void callback_request(const setup_packet_t *setup) override;
    void callback_request_xfer_done(const setup_packet_t *setup, bool aborted) override;
    void callback_set_configuration(uint8_t configuration) override;
    void callback_set_interface(uint16_t interface, uint8_t alternate) override;

private:
    static const uint8_t default_configuration = 1;
    static const size_t configuration_descriptor_length = 18;
    uint8_t configuration_descriptor[configuration_descriptor_length];
};

MyUSBDevice::MyUSBDevice() : USBDevice(get_usb_phy(), 0x1f00, 0x2012, 0x0001) {
    connect();
}

MyUSBDevice::~MyUSBDevice() {
    deinit();
}

const uint8_t *MyUSBDevice::configuration_desc(uint8_t index) {
    const uint8_t configuration_descriptor_temp[configuration_descriptor_length] = {
        // configuration descriptor, USB spec 9.6.3
        CONFIGURATION_DESCRIPTOR_LENGTH, // bLength
        CONFIGURATION_DESCRIPTOR, // bDescriptorType
        LSB(configuration_descriptor_length), // wTotalLength
        MSB(configuration_descriptor_length),
        1,                      // bNumInterfaces
        default_configuration,  // bConfigurationValue
        0,                      // iConfiguration
        C_RESERVED,             // bmAttributes
        C_POWER(100),           // bMaxPower

        // interface descriptor, USB spec 9.6.5
        INTERFACE_DESCRIPTOR_LENGTH, // bLength
        INTERFACE_DESCRIPTOR,   // bDescriptorType
        0,                      // bInterfaceNumber
        0,                      // bAlternateSetting
        0,                      // bNumEndpoints
        0xff,                   // bInterfaceClass
        0xff,                   // bInterfaceSubClass
        0xff,                   // bInterfaceProtocol
        0,                      // iInterface
    };

    if (index == 0) {
        MBED_ASSERT(sizeof(configuration_descriptor_temp) == sizeof(configuration_descriptor));
        memcpy(configuration_descriptor, configuration_descriptor_temp, sizeof(configuration_descriptor));
        return &configuration_descriptor[0];
    } else {
        return nullptr;
    }
}

void MyUSBDevice::callback_state_change(DeviceState new_state) {
    assert_locked();
}

void MyUSBDevice::callback_request(const setup_packet_t *setup) {
    assert_locked();

    if (setup->bmRequestType.Type == VENDOR_TYPE) {
        if (setup->bmRequestType.dataTransferDirection == 0) {
            MBED_ASSERT(sizeof(received_request_data) >= setup->wLength);
            complete_request(Receive, &received_request_data[0], setup->wLength);
        } else if (setup->bmRequestType.dataTransferDirection == 1) {
            // "If the USB component calls complete_request with a buffer and size,
            // that buffer must remain valid and unchanged until USBDevice calls the function callback_request_xfer_done."
            // The easiest way to achieve this is to make it static.
            static uint8_t send_request_data[] = { 's', 'e', 'n', 'd', ' ', 'r', 'e', 'q', 'u', 'e', 's', 't', '\0' };
            // The transfer might not request the entire buffer, which is fine.
            const auto size = std::min<uint32_t>(setup->wLength, sizeof(send_request_data));
            complete_request(Send, &send_request_data[0], size);
        } else {
            MBED_ASSERT(false);
        }
    } else {
        complete_request(PassThrough);
    }
}

void MyUSBDevice::callback_request_xfer_done(const setup_packet_t *setup, bool aborted) {
    assert_locked();
    complete_request_xfer_done(!aborted);
}

void MyUSBDevice::callback_set_configuration(uint8_t configuration) {
    assert_locked();
    complete_set_configuration(true);
}

void MyUSBDevice::callback_set_interface(uint16_t interface, uint8_t alternate) {
    assert_locked();
    complete_set_interface(true);
}

}

int main() {
    printf("usb-device\n");

    MyUSBDevice my_usb_device;

    while (1) {
        printf("still running...\n");
        rtos::ThisThread::sleep_for(1000);
    }
}
