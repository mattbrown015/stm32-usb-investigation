#include <libusb-1.0/libusb.h>

#include <cassert>
#include <cinttypes>
#include <cstdio>

namespace
{

void print_libusb_error(const libusb_error error, const char *const libusb_api_function)  {
    printf("'%s' failed, error value %d, error name '%s', error description '%s'\n", libusb_api_function, error, libusb_error_name(error), libusb_strerror(error));
}

void print_device_list(libusb_device **device_list) {
    puts("Print USB device list");

    libusb_device *device;
    int i = 0;

    while ((device = device_list[i++]) != NULL) {
        struct libusb_device_descriptor device_descriptor;
        const auto error = libusb_get_device_descriptor(device, &device_descriptor);
        if (error < 0) {
            print_libusb_error(static_cast<libusb_error>(error), "libusb_get_device_descriptor");
            return;
        }

        printf("\t%04x:%04x (bus %d, device %d)",
            device_descriptor.idVendor, device_descriptor.idProduct,
            libusb_get_bus_number(device), libusb_get_device_address(device));

        uint8_t path[8];
        const auto port_number = libusb_get_port_numbers(device, path, sizeof(path));
        if (port_number > 0) {
            printf(" path: %d", path[0]);
            for (int j = 1; j < port_number; j++)
                printf(".%d", path[j]);
        }
        putchar('\n');
    }
}

libusb_device_handle* open_device(libusb_device **device_list, const uint16_t idVendor, const uint16_t idProduct) {
    libusb_device_handle *device_handle = NULL;

    libusb_device *device_found = NULL;
    for (auto i = 0; device_list[i]; i++) {
        libusb_device *device = device_list[i];
        libusb_device_descriptor device_descriptor;
        auto error = libusb_get_device_descriptor(device, &device_descriptor);
        if (error < 0) {
            print_libusb_error(static_cast<libusb_error>(error), "libusb_get_device_descriptor");
        }
        if (device_descriptor.idVendor == idVendor && device_descriptor.idProduct == idProduct) {
            if (device_found == NULL) {
                device_found = device;
            } else {
                puts("Found another device with matching 'idVendor' and 'idProduct', only 1 device is supported at present.");
            }
        }
    }

    if (device_found) {
        printf("found device with idVendor 0x%" PRIx16 " idProduct 0x%" PRIx16 "\n", idVendor, idProduct);

        const auto error = libusb_open(device_found, &device_handle);
        if (error) {
            print_libusb_error(static_cast<libusb_error>(error), "libusb_open");
            if (error == LIBUSB_ERROR_NOT_SUPPORTED) {
                puts("'Operation not supported' error probably means Windows hasn't found a compatible driver for this device.");
                puts("Use Zadig, https://zadig.akeo.ie/, to install the WinUSB driver for this device.");
            }
        }

    } else {
        printf("failed to find device with idVendor 0x%" PRIx16 " idProduct 0x%" PRIx16 "\n", idVendor, idProduct);
    }

    return device_handle;
}

bool check_configuration_value(libusb_device_handle *const device_handle) {
    bool success = false;

    int configuration_value = 0;
    const auto error = libusb_get_configuration(device_handle, &configuration_value);
    if (error) {
        print_libusb_error(static_cast<libusb_error>(error), "libusb_get_configuration");
    } else {
        if (configuration_value == 1) {
            success = true;
        } else {
            printf("'libusb_get_configuration' succeeded, configuration value is %d", configuration_value);
        }
    }

    return success;
}

void do_somthing_with_device(libusb_device_handle *const device_handle) {
    assert(device_handle);

    if (!check_configuration_value(device_handle)) return;
}

}

int main() {
    puts("usb-host");

    const auto error = libusb_init(NULL);
    if (error < 0) {
        print_libusb_error(static_cast<libusb_error>(error), "libusb_init");
        return 1;
    }

    libusb_device **device_list;
    const auto number_of_devices = libusb_get_device_list(NULL, &device_list);
    printf("number of devices %lld\n", number_of_devices);
    if (number_of_devices < 0) {
        libusb_exit(NULL);
        return 1;
    }

    print_device_list(device_list);

    auto device_handle = open_device(device_list, 0x1f00, 0x2012);

    libusb_free_device_list(device_list, 1);

    do_somthing_with_device(device_handle);

    libusb_close(device_handle);

    libusb_exit(NULL);

    return 0;
}
