#include "evk-usb-device-hal.h"
#include "show-running.h"
#include "spi-rx.h"

#include <rtos/ThisThread.h>

#include <cstdio>

int main() {
    puts("usb-device-use-hal");

    show_running::init();

    spi_rx::init();
    evk_usb_device_hal::init();

    rtos::ThisThread::sleep_for(rtos::Kernel::wait_for_u32_forever);
}
