#include "trace.h"

#include "serial-mutex.h"

#include <features/frameworks/mbed-trace/mbed-trace/mbed_trace.h>

namespace trace {

void init() {
    mbed_trace_mutex_wait_function_set(serial_mutex::out_lock);
    mbed_trace_mutex_release_function_set(serial_mutex::out_unlock);

    mbed_trace_init();
}

}
