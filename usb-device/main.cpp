#include <drivers/internal/USBDescriptor.h>
#include <drivers/internal/USBDevice.h>
#include <hal/usb/usb_phy_api.h>
#include <rtos/ThisThread.h>

#include <cstdio>

namespace
{

class MyUSBDevice : public USBDevice {
public:
    MyUSBDevice();
    ~MyUSBDevice();

protected:
    const uint8_t *configuration_desc(uint8_t index);

    void callback_state_change(DeviceState new_state);
    void callback_request(const setup_packet_t *setup);
    void callback_request_xfer_done(const setup_packet_t *setup, bool aborted);
    void callback_set_configuration(uint8_t configuration);
    void callback_set_interface(uint16_t interface, uint8_t alternate);
};

MyUSBDevice::MyUSBDevice() : USBDevice(get_usb_phy(), 0x1f00, 0x2012, 0x0001) {
    connect();
}

MyUSBDevice::~MyUSBDevice() {
    deinit();
}

const uint8_t *MyUSBDevice::configuration_desc(uint8_t index) {
    const uint8_t default_configuration = 1;
    const size_t configuration_descriptor_length = 18;
    static const uint8_t configuration_descriptor[configuration_descriptor_length] = {
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
        0x00,                   // bInterfaceSubClass
        0xff,                   // bInterfaceProtocol
        0,                      // iInterface
    };

    return &configuration_descriptor[0];
}

void MyUSBDevice::callback_state_change(DeviceState new_state) {

}

void MyUSBDevice::callback_request(const setup_packet_t *setup) {
    complete_request(PassThrough, NULL, 0);
}

void MyUSBDevice::callback_request_xfer_done(const setup_packet_t *setup, bool aborted) {
    complete_request_xfer_done(!aborted);
}

void MyUSBDevice::callback_set_configuration(uint8_t configuration) {
    complete_set_configuration(true);
}

void MyUSBDevice::callback_set_interface(uint16_t interface, uint8_t alternate) {
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
