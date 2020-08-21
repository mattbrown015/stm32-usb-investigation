#include "command-line.h"

#include "buffers.h"
#include "serial-mutex.h"
#include "version-string.h"

#include <features/frameworks/mbed-client-cli/mbed-client-cli/ns_cmdline.h>
#include <rtos/Thread.h>

namespace command_line
{

namespace
{

const size_t stack_size = OS_STACK_SIZE; // /Normal/ stack size
MBED_ALIGN(8) unsigned char stack[stack_size];
rtos::Thread thread(osPriorityNormal, sizeof(stack), stack, "command-line");

int print_buffer(int argc, char *argv[]) {
    if (argc > 1) {
        const auto index = strtoul(argv[1], nullptr, 0);
        if (index < buffers::number_of) {
            buffers::print_buffer(index);
            return CMDLINE_RETCODE_SUCCESS;
        } else {
            return CMDLINE_RETCODE_INVALID_PARAMETERS;
        }
    } else {
        return CMDLINE_RETCODE_INVALID_PARAMETERS;
    }
}

int version_information(int argc, char *argv[]) {
    cmd_printf("%s\n", version_string);
    cmd_printf("%s\n", mbed_os_version_string);
    return CMDLINE_RETCODE_SUCCESS;
}

void command_line() {
    while(true) {
        const auto c = getchar();
        if (c != EOF) {
            cmd_char_input(c);
        }
    }
}

}

void init() {
    cmd_init(nullptr);
    cmd_mutex_wait_func(serial_mutex::out_lock);
    cmd_mutex_release_func(serial_mutex::out_unlock);

    cmd_add("printf-buffer", print_buffer, "Print SPI rx buffer", "Print contents of specified SPI rx buffer\nprint-buffer <0..4>\nConcurrency issues exist if the SPI if the SPI master is running");
    cmd_alias_add("pb", "printf-buffer");
    cmd_add("version", version_information, "version information", nullptr);
    cmd_alias_add("ver", "version");

    MBED_UNUSED const auto os_status = thread.start(command_line);
    MBED_ASSERT(os_status == osOK);
}

}
