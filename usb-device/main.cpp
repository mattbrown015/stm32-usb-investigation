#if !defined(DEVICE_USBDEVICE)

#include "evk-usb-device-hal.h"
#include "show-running.h"

#include <rtos/ThisThread.h>

#include <cstdio>

int main() {
    puts("usb-device-use-hal");

    show_running::init();

    evk_usb_device_hal::init();

    rtos::ThisThread::sleep_for(rtos::Kernel::wait_for_u32_forever);
}

#else

#include "evk-usb-device.h"
#include "show-running.h"
#include "usb-device.h"

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

void repeat_bulk_in_transfer() {
    printf("perform %d bulk in transfers\n", usb_device::number_of_bulk_in_repeats);
    std::array<unsigned char, 64> buffer;
    std::iota(std::begin(buffer), std::end(buffer), 1);

    for (auto i = 0; i < usb_device::number_of_bulk_in_repeats; ++i) {
        const auto number_of_bytes_transferred = evk_usb_device.bulk_in_transfer(buffer.data(), buffer.size());
        if (number_of_bytes_transferred != buffer.size()) {
            printf("Unexpected number of bytes transferred %" PRIu32 "\n", number_of_bytes_transferred);
        }
    }

    printf("completed %d bulk in transfers\n", usb_device::number_of_bulk_in_repeats);
}

void wait_until_configured() {
    evk_usb_device.wait_configured();
    puts("EvkUSBDevice configured");

    repeat_bulk_in_transfer();

    puts("perform bulk out transfer");
    std::array<unsigned char, 64> buffer{ 0 };
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

    show_running::init();

    MBED_UNUSED const osStatus status = thread.start(wait_until_configured);
    MBED_ASSERT(status == osOK);

    rtos::ThisThread::sleep_for(rtos::Kernel::wait_for_u32_forever);
}

#endif
