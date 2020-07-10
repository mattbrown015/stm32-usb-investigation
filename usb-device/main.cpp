#include "evk-usb-device.h"

#include <rtos/ThisThread.h>
#include <rtos/Thread.h>

#include <array>
#include <cinttypes>
#include <cstdio>
#include <numeric>

namespace
{

const size_t stack_size = 2048;
MBED_ALIGN(8) unsigned char stack[stack_size];
rtos::Thread thread(osPriorityNormal, sizeof(stack), stack, "wait_until_configured");

EvkUSBDevice::EvkUSBDevice evk_usb_device;

void wait_until_configured() {
    evk_usb_device.wait_configured();
    puts("EvkUSBDevice configured");

    puts("perform bulk in transfer");
    std::array<unsigned char, 64> buffer;
    std::iota(std::begin(buffer), std::end(buffer), 1);
    const auto number_of_bytes_transferred = evk_usb_device.bulk_in_transfer(buffer.data(), buffer.size());
    printf("number_of_bytes_transferred %" PRIi32 "\n", number_of_bytes_transferred);

    puts("perform bulk out transfer");
    buffer.fill(0);
    const auto out_number_of_bytes_transferred = evk_usb_device.bulk_out_transfer(buffer.data(), buffer.size());
    printf("out_number_of_bytes_transferred %" PRIi32 "\n", out_number_of_bytes_transferred);

    for (auto i = 0u; i < buffer.size(); ++i) {
        printf("0x%02" PRIx8 " ", buffer[i]);
    }
    putchar('\n');
}

}

int main() {
    puts("usb-device");

    MBED_UNUSED const osStatus status = thread.start(wait_until_configured);
    MBED_ASSERT(status == osOK);

    while (1) {
        puts("still running...");
        rtos::ThisThread::sleep_for(1000);
    }
}
