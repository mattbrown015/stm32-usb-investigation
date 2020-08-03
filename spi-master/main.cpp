#include "show-running.h"

#include <rtos/ThisThread.h>

#include <cstdio>

int main() {
    puts("spi-master");

    show_running::init();

    rtos::ThisThread::sleep_for(rtos::Kernel::wait_for_u32_forever);
}
