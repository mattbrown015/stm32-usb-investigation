#include <drivers/USBCDC.h>
#include <rtos/ThisThread.h>

#include <cstdio>

USBCDC cdc;

int main() {
    printf("usb-device\n");

    while (1) {
        char msg[] = "Hello world\r\n";
        cdc.send((uint8_t *)msg, strlen(msg));
        rtos::ThisThread::sleep_for(1000);
    }
}
