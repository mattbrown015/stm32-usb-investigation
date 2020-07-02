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
    return NULL;
}

void MyUSBDevice::callback_state_change(DeviceState new_state) {

}

void MyUSBDevice::callback_request(const setup_packet_t *setup) {

}

void MyUSBDevice::callback_request_xfer_done(const setup_packet_t *setup, bool aborted) {

}

void MyUSBDevice::callback_set_configuration(uint8_t configuration) {

}

void MyUSBDevice::callback_set_interface(uint16_t interface, uint8_t alternate) {

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
