#pragma once

#include <platform/mbed_toolchain.h>

#include <cinttypes>
#include <cstddef>

namespace buffers
{

const size_t number_of = 4;
const size_t size_of = 512;

uint8_t *get_empty_buffer();
void set_buffer_empty(uint8_t *const buffer_ptr);
uint8_t *get_full_buffer();
MBED_DEPRECATED("Added only to check SPI rx data on the Disco, the buffer returned could be overwritten at any time.")
uint8_t *peek_full_buffer();
void set_buffer_full(uint8_t *const buffer_ptr);

void init();

}
