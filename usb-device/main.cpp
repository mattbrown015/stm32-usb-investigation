#include "evk-usb-device.h"

#include <rtos/ThisThread.h>

#include <cstdio>

int main() {
    printf("usb-device\n");

    EvkUSBDevice::EvkUSBDevice evk_usb_device;

    while (1) {
        printf("still running...\n");
        rtos::ThisThread::sleep_for(1000);
    }
}
