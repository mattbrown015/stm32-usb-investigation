#include <libusb-1.0/libusb.h>

#include <cstdio>
#include <cinttypes>

namespace
{

void print_device_list(libusb_device **device_list) {
    libusb_device *device;
    int i = 0;

    while ((device = device_list[i++]) != NULL) {
        struct libusb_device_descriptor device_descriptor;
        const auto error = libusb_get_device_descriptor(device, &device_descriptor);
        if (error < 0) {
            printf("failed to get device descriptor %s\n", libusb_strerror(static_cast<libusb_error>(error)));
            return;
        }

        printf("%04x:%04x (bus %d, device %d)",
            device_descriptor.idVendor, device_descriptor.idProduct,
            libusb_get_bus_number(device), libusb_get_device_address(device));

        uint8_t path[8];
        const auto port_number = libusb_get_port_numbers(device, path, sizeof(path));
        if (port_number > 0) {
            printf(" path: %d", path[0]);
            for (int j = 1; j < port_number; j++)
                printf(".%d", path[j]);
        }
        printf("\n");
    }
}

void open_device(libusb_device **device_list, const uint16_t idVendor, const uint16_t idProduct) {
    libusb_device *device_found = NULL;
    for (auto i = 0; device_list[i]; i++) {
        libusb_device *device = device_list[i];
        libusb_device_descriptor device_descriptor;
        auto error = libusb_get_device_descriptor(device, &device_descriptor);
        if (error < 0) {
            printf("failed to get desc %d %s\n", error, libusb_strerror(static_cast<libusb_error>(error)));
        }
        if (device_descriptor.idVendor == idVendor && device_descriptor.idProduct == idProduct) {
            device_found = device;
            break;
        }
    }

    if (device_found) {
        printf("found device with idVendor 0x%" PRIx16 " idProduct 0x%" PRIx16 "\n", idVendor, idProduct);
        libusb_device_handle *device_handle;

        const auto error = libusb_open(device_found, &device_handle);
        if (error) {
            printf("failed to open %d %s\n", error, libusb_strerror(static_cast<libusb_error>(error)));
            return;
        }

        libusb_close(device_handle);
    } else {
        printf("failed to find device with idVendor 0x%" PRIx16 " idProduct 0x%" PRIx16 "\n", idVendor, idProduct);
    }
}

}

int main() {
    printf("usb-host\n");

    const auto error = libusb_init(NULL);
    if (error < 0) {
        printf("libusb_init failed %d %s\n", error, libusb_strerror(static_cast<libusb_error>(error)));
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

    print_device_list(device_list);

    open_device(device_list, 0x1f00, 0x2012);

    libusb_free_device_list(device_list, 1);

    libusb_exit(NULL);

    return 0;
}
