#include "command-line.h"

#include "buffers.h"
#include "main.h"
#include "version-string.h"

#include <rtos/Thread.h>

#include <iostream>
#include <sstream>
#include <vector>

namespace command_line
{

namespace
{

const size_t stack_size = OS_STACK_SIZE; // /Normal/ stack size
MBED_ALIGN(8) unsigned char stack[stack_size];
rtos::Thread thread(osPriorityNormal, sizeof(stack), stack, "command-line");

void command_line() {
    while (1) {
        std::string line;
        std::getline(std::cin, line);

        std::istringstream iss{line};
        std::vector<std::string> tokenized_line{std::istream_iterator<std::string>{iss}, std::istream_iterator<std::string>{}};
        if (tokenized_line.size() > 0) {
            if (tokenized_line[0] == "ver") {
                puts(version_string);
                puts(mbed_os_version_string);
            } else if (tokenized_line[0] == "print-buffer") {
                if (tokenized_line.size() > 1) {
                    const auto index = std::stoul(tokenized_line[1], 0, 0);
                    if (index < buffers::number_of) {
                        event_queue.call(buffers::print_buffer, index);
                    } else {
                        printf("Invalid parameter: index must be less than %u\n", buffers::number_of);
                    }
                } else {
                    puts("Too few parameters");
                }
            } else {
                puts("command not recognised");
            }
        }
    }
}

}

void init() {
    MBED_UNUSED const auto os_status = thread.start(command_line);
    MBED_ASSERT(os_status == osOK);
}

}
