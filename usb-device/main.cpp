#include "main.h"

#include "buffers.h"
#include "evk-usb-device-hal.h"
#include "show-running.h"
#include "spi-rx.h"

#include <cstdio>

MBED_ALIGN(4) unsigned char event_queue_buffer[EVENTS_QUEUE_SIZE];
events::EventQueue event_queue(EVENTS_QUEUE_SIZE, &event_queue_buffer[0]);

int main() {
    puts("usb-device-use-hal");

    show_running::init();

    buffers::init();  // Initialise the buffers first because the SPI will want an empty buffer during its initialisation.
    spi_rx::init();
    evk_usb_device_hal::init();

    event_queue.dispatch_forever();
}
