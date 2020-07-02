#include <libusb-1.0/libusb.h>

#include <cstdio>

int main() {
    printf("usb-host\n");

    const auto r = libusb_init(NULL);
    if (r < 0) {
        printf("libusb_init failed %d\n", r);
        return 1;
    } else {
        printf("libusb_init success\n");
    }

    libusb_exit(NULL);

    return 0;
}
