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
    evk_usb_device.wait_configured();

    puts("EvkUSBDevice configured");
}

}

int main() {
    puts("usb-device");

    MBED_UNUSED const osStatus status = thread.start(wait_until_configured);
    MBED_ASSERT(status == osOK);

    while (1) {
        puts("still running...");
        rtos::ThisThread::sleep_for(1000);
    }
}
