#pragma once

#include <events/EventQueue.h>

void run_dma_for(const unsigned long number_of_buffers);
void toggle_dma();

extern events::EventQueue event_queue;
