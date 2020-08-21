#include "serial-mutex.h"

#include <rtos/Mutex.h>

namespace serial_mutex {

namespace
{

rtos::Mutex out;

}

void out_lock() {
    out.lock();
}

void out_unlock() {
    out.unlock();
}

}
