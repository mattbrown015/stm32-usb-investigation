#include "buffers.h"

#include <features/frameworks/mbed-client-cli/mbed-client-cli/ns_cmdline.h>
#include <platform/mbed_assert.h>
#include <rtos/Mail.h>
#include <rtos/Kernel.h>

namespace buffers
{

namespace
{

// I think this could have been achieved with a single mail box queue
// if the clients understood the mail type and remembered the messages.
// I believe using 2 queues has allowed the clients, SPI and USB layers,
// to just deal in buffer pointers which the remember naturally.
//
// Overview...
//     The empty buffer queue is initialised with pointers to all the buffers
//     allocated in the buffer array. The full buffer queue starts off with
//     nothing in it.
//     The SPI layer takes a buffer off the empty buffer queue, fills it and
//     puts it on the full buffer queue. This is done within the ISR context
//     because it is important the SPI DMA destination pointers get updated
//     quickly. If there are no empty buffers available the SPI layer will
//     set the DMA destination to some overflow buffers. In other words,
//     the SPI runs continually and the data gets lost if the USB isn't keeping up.
//     When the USB layer is ready to transmit it attempts to get a full buffer.
//     If there isn't a full buffer available it will block.

uint8_t buffer[number_of][size_of] = { { 0 } };

struct mail_t {
    uint8_t *buffer_ptr;
};

rtos::Mail<mail_t, number_of> empty_buffers_queue;
rtos::Mail<mail_t, number_of> full_buffers_queue;

#ifndef NDEBUG
// Perhaps this should be class to avoid this kind of nonsense.
bool initialised = false;
#endif

}

uint8_t *get_empty_buffer() {
    MBED_ASSERT(initialised);

    uint8_t *buffer_ptr = nullptr;
    const auto mail = empty_buffers_queue.try_get();
    if (mail != nullptr) {
        buffer_ptr = mail->buffer_ptr;
        empty_buffers_queue.free(mail);
    }
    return buffer_ptr;
}

void set_buffer_empty(uint8_t *const buffer_ptr) {
    MBED_ASSERT(initialised);

    const auto mail = empty_buffers_queue.try_alloc();
    MBED_ASSERT(mail != nullptr);
    mail->buffer_ptr = buffer_ptr;
    empty_buffers_queue.put(mail);
}

uint8_t *get_full_buffer() {
    MBED_ASSERT(initialised);

    const auto mail = full_buffers_queue.try_get_for(rtos::Kernel::wait_for_u32_forever);
    const auto buffer_ptr = mail->buffer_ptr;
    full_buffers_queue.free(mail);
    return buffer_ptr;
}

uint8_t *peek_full_buffer() {
    MBED_ASSERT(initialised);

    uint8_t *buffer_ptr = nullptr;
    const auto mail = full_buffers_queue.try_get();
    if (mail != nullptr) {
        buffer_ptr = mail->buffer_ptr;
    }
    return buffer_ptr;
}

void set_buffer_full(uint8_t *const buffer_ptr) {
    MBED_ASSERT(initialised);

    const auto mail = full_buffers_queue.try_alloc();
    MBED_ASSERT(mail != nullptr);
    mail->buffer_ptr = buffer_ptr;
    full_buffers_queue.put(mail);
}

void print_buffer(const size_t index) {
    MBED_ASSERT(index < number_of);
    const uint32_t *word_ptr = reinterpret_cast<uint32_t*>(&buffer[index][0]);
    for (auto i = 0u; i < size_of / sizeof(uint32_t); ++i) {
        cmd_printf("0x%" PRIx32 " ", *word_ptr);
        ++word_ptr;

        if (((i + 1) % 8) == 0) cmd_printf("\n");
    }
}

void init() {
#ifndef NDEBUG
    initialised = true;
#endif

    for (auto i = 0u; i < number_of; ++i) {
        set_buffer_empty(&buffer[i][0]);
    }
}

}
