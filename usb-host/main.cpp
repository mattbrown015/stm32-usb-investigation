#include <libusb-1.0/libusb.h>

#include <cstdio>

namespace
{

void print_devs(libusb_device **device_list)
{
    libusb_device *device;
    int i = 0, j = 0;
    uint8_t path[8];

    while ((device = device_list[i++]) != NULL) {
        struct libusb_device_descriptor device_descriptor;
        int r = libusb_get_device_descriptor(device, &device_descriptor);
        if (r < 0) {
            printf("failed to get device descriptor\n");
            return;
        }

        printf("%04x:%04x (bus %d, device %d)",
            device_descriptor.idVendor, device_descriptor.idProduct,
            libusb_get_bus_number(device), libusb_get_device_address(device));

        r = libusb_get_port_numbers(device, path, sizeof(path));
        if (r > 0) {
            printf(" path: %d", path[0]);
            for (j = 1; j < r; j++)
                printf(".%d", path[j]);
        }
        printf("\n");
    }
}

}

int main() {
    printf("usb-host\n");

    const auto r = libusb_init(NULL);
    if (r < 0) {
        printf("libusb_init failed %d\n", r);
        return 1;
    } else {
        printf("libusb_init success\n");
    }

    libusb_device **device_list;
    const auto number_of_devices = libusb_get_device_list(NULL, &device_list);
    printf("number of devices %lld\n", number_of_devices);
    if (number_of_devices < 0) {
        libusb_exit(NULL);
        return 1;
    }

    print_devs(device_list);
    libusb_free_device_list(device_list, 1);

    libusb_exit(NULL);

    return 0;
}
