#include "main.h"

#include "buffers.h"
#include "command-line.h"
#include "evk-usb-device-hal.h"
#include "show-running.h"
#include "spi-rx.h"
#include "trace.h"
#include "version-string.h"

#include <features/frameworks/mbed-trace/mbed-trace/mbed_trace.h>

#include <cstdio>

#define TRACE_GROUP "main"

MBED_ALIGN(4) unsigned char event_queue_buffer[EVENTS_QUEUE_SIZE];
events::EventQueue event_queue(EVENTS_QUEUE_SIZE, &event_queue_buffer[0]);

int main() {
    trace::init();
    show_running::init();
    buffers::init();  // Initialise the buffers first because the SPI will want an empty buffer during its initialisation.
    spi_rx::init();
    evk_usb_device_hal::init();
    command_line::init();

    event_queue.dispatch_forever();
}
