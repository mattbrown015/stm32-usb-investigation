#include "show-running.h"

#include <drivers/DigitalOut.h>
#include <rtos/ThisThread.h>
#include <rtos/Thread.h>

namespace show_running
{

namespace
{

mbed::DigitalOut green_led(LED1);

const size_t stack_size = 512; // Arbitrarily /small/ stack
MBED_ALIGN(8) unsigned char stack[stack_size];
rtos::Thread thread(osPriorityLow, sizeof(stack), stack, "show_running");

void show_running() {
    while (1) {
        green_led = !green_led;

        rtos::ThisThread::sleep_for(1000);
    }
}

}

void init() {
    MBED_UNUSED const auto status = thread.start(show_running);
    MBED_ASSERT(status == osOK);
}

}
