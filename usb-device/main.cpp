#include "evk-usb-device.h"

#include <rtos/ThisThread.h>
#include <rtos/Thread.h>

#include <cstdio>

namespace
{

const size_t stack_size = 2048;
MBED_ALIGN(8) unsigned char stack[stack_size];
rtos::Thread thread(osPriorityNormal, sizeof(stack), stack, "wait_until_configured");

EvkUSBDevice::EvkUSBDevice evk_usb_device;

void wait_until_configured() {
    evk_usb_device.configured.acquire();

    printf("EvkUSBDevice configured\n");
}

}

int main() {
    printf("usb-device\n");

    MBED_UNUSED const osStatus status = thread.start(wait_until_configured);
    MBED_ASSERT(status == osOK);

    while (1) {
        printf("still running...\n");
        rtos::ThisThread::sleep_for(1000);
    }
}
