#include "command-line.h"

#include "main.h"
#include "version-string.h"

#include <rtos/Thread.h>

#include <iostream>

namespace command_line
{

namespace
{

const size_t stack_size = OS_STACK_SIZE; // /Normal/ stack size
MBED_ALIGN(8) unsigned char stack[stack_size];
rtos::Thread thread(osPriorityNormal, sizeof(stack), stack, "command-line");

void command_line() {
    while (1) {
        std::string s;
        std::getline(std::cin, s);

        if (s == "ver") {
            puts(version_string);
            puts(mbed_os_version_string);
        } else if (s == "toggle-dma") {
            event_queue.call(toggle_dma);
        } else {
            puts("command not recognised");
        }
    }
}

}

void init() {
    MBED_UNUSED const auto os_status = thread.start(command_line);
    MBED_ASSERT(os_status == osOK);
}

}
