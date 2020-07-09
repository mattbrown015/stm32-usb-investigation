#include "evk-usb-device.h"

#include <rtos/ThisThread.h>

#include <cstdio>

namespace
{

EvkUSBDevice::EvkUSBDevice evk_usb_device;

}

int main() {
    printf("usb-device\n");

    while (1) {
        printf("still running...\n");
        rtos::ThisThread::sleep_for(1000);
    }
}
