#include "command-line.h"

#include "main.h"
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

int changing_data_callback(int argc, char *argv[]) {
    event_queue.call(changing_data);
    return CMDLINE_RETCODE_SUCCESS;
}

int constant_data_callback(int argc, char *argv[]) {
    event_queue.call(constant_data);
    return CMDLINE_RETCODE_SUCCESS;
}

int dma_size_callback(int argc, char *argv[]) {
    if (argc > 1) {
        const auto size = strtoul(argv[1], nullptr, 0);
        if (size <= 256) {
            event_queue.call(set_dma_size, size);
            return CMDLINE_RETCODE_SUCCESS;
        } else {
            return CMDLINE_RETCODE_INVALID_PARAMETERS;
        }
    } else {
        return CMDLINE_RETCODE_INVALID_PARAMETERS;
    }
    return CMDLINE_RETCODE_SUCCESS;
}

int print_tx_buffer_callback(int argc, char *argv[]) {
    print_tx_buffers();
    return CMDLINE_RETCODE_SUCCESS;
}

int run_dma_for_callback(int argc, char *argv[]) {
    if (argc > 1) {
        const auto number_of_buffers = strtoul(argv[1], nullptr, 0);
        event_queue.call(run_dma_for, number_of_buffers);
        return CMDLINE_RETCODE_SUCCESS;
    } else {
        return CMDLINE_RETCODE_INVALID_PARAMETERS;
    }
    return CMDLINE_RETCODE_SUCCESS;
}

int toggle_dma_callback(int argc, char *argv[]) {
    event_queue.call(toggle_dma);
    return CMDLINE_RETCODE_SUCCESS;
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

    cmd_add("changing-data", changing_data_callback, "Transmit Changing Data", nullptr);
    cmd_add("constant-data", constant_data_callback, "Transmit Constant Data", nullptr);
    cmd_add("dma-size", dma_size_callback, "Set DMA size", "Set the size of the SPI DMA transfers\ndma-size <size>");
    cmd_add("print-tx-buffer", print_tx_buffer_callback, "Print tx buffer contents", nullptr);
    cmd_alias_add("pt", "print-tx-buffer");
    cmd_add("run-dma-for", run_dma_for_callback, "Run DMA for specified number of buffers", "Run DMA for specified number of buffer, this makes it easier to check the buffer contents\nrun-dma-for <num buffers>");
    cmd_alias_add("rf", "run-dma-for");
    cmd_add("toggle-dma", toggle_dma_callback, "toggle DMA running", nullptr);
    cmd_alias_add("td", "toggle-dma");
    cmd_add("version", version_information, "version information", nullptr);
    cmd_alias_add("ver", "version");

    MBED_UNUSED const auto os_status = thread.start(command_line);
    MBED_ASSERT(os_status == osOK);
}

}
