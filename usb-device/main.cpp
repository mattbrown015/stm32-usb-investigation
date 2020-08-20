#include "main.h"

#include "buffers.h"
#include "evk-usb-device-hal.h"
#include "show-running.h"
#include "spi-rx.h"

#include <platform/mbed_version.h>

#include <cstdio>

MBED_ALIGN(4) unsigned char event_queue_buffer[EVENTS_QUEUE_SIZE];
events::EventQueue event_queue(EVENTS_QUEUE_SIZE, &event_queue_buffer[0]);

int main() {
    puts("usb-device");
    printf("Mbed OS version %d.%d.%d\n", MBED_MAJOR_VERSION, MBED_MINOR_VERSION, MBED_PATCH_VERSION);

    show_running::init();

    buffers::init();  // Initialise the buffers first because the SPI will want an empty buffer during its initialisation.
    spi_rx::init();
    evk_usb_device_hal::init();

    event_queue.dispatch_forever();
}
