#pragma once

#include <events/EventQueue.h>

void print_tx_buffers();
void run_dma_for(const unsigned long number_of_buffers);
void set_dma_size(const uint16_t size);
void toggle_dma();

extern events::EventQueue event_queue;
