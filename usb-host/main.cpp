#include <libusb-1.0/libusb.h>

#include <cassert>
#include <cinttypes>
#include <cstdio>

namespace
{

const uint8_t invalid_ep_address = 0xff;
uint8_t epbulk_in = invalid_ep_address;

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

bool get_endpoint_addresses(libusb_device *const device) {
    assert(device);

    auto success = false;

    libusb_config_descriptor *config_descriptor = nullptr;
    const libusb_interface_descriptor *interface_descriptor = nullptr;
    const libusb_endpoint_descriptor *endpoint_descriptor = nullptr;

    const auto error = libusb_get_config_descriptor(device, 0, &config_descriptor);
    if (error < 0) {
        print_libusb_error(static_cast<libusb_error>(error), "libusb_get_device_descriptor");
        return false;
    }

    if (config_descriptor->bNumInterfaces != 1) {
        printf("unexpected number of interfaces %" PRIi8 "\n", config_descriptor->bNumInterfaces);
        goto free_and_exit;
    }
    if (config_descriptor->interface->num_altsetting != 1) {
        printf("unexpected number of alternate settings %" PRIi8 "\n", config_descriptor->interface->num_altsetting);
        goto free_and_exit;
    }

    interface_descriptor = config_descriptor->interface->altsetting;

    if (interface_descriptor->bNumEndpoints != 2) {
        printf("unexpected number of endpoints %" PRIi8 "\n", interface_descriptor->bNumEndpoints);
        goto free_and_exit;
    }

    endpoint_descriptor = config_descriptor->interface->altsetting->endpoint;
    {
        const auto bEndpointAddress = endpoint_descriptor->bEndpointAddress;
        const auto bmAttributes = endpoint_descriptor->bmAttributes;
        printf("bEndpointAddress 0x%" PRIx8 " bmAttributes 0x%" PRIx8 "\n", bEndpointAddress, bmAttributes);

        const bool is_in = (bEndpointAddress & LIBUSB_ENDPOINT_IN) == LIBUSB_ENDPOINT_IN;
        const bool is_bulk = bmAttributes == LIBUSB_TRANSFER_TYPE_BULK;
        if (!is_in || !is_bulk) {
            puts("endpoint is not 'in' and/or not 'bulk'");
            goto free_and_exit;
        }
        epbulk_in = bEndpointAddress;
    }

    success = true;
free_and_exit:
    libusb_free_config_descriptor(config_descriptor);
    return success;
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

        if (!get_endpoint_addresses(device_found)) {
            return 0;
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

bool claim_interface(libusb_device_handle *const device_handle) {
    const auto error = libusb_claim_interface(device_handle, 0);
    if (error) {
        print_libusb_error(static_cast<libusb_error>(error), "libusb_claim_interface");
        return false;
    } else {
        return true;
    }
}

bool release_interface(libusb_device_handle *const device_handle) {
    const auto error = libusb_release_interface(device_handle, 0);
    if (error) {
        print_libusb_error(static_cast<libusb_error>(error), "libusb_release_interface");
        return false;
    } else {
        return true;
    }
}

bool control_transfer_out(libusb_device_handle *const device_handle) {
    unsigned char data[] = { 's', 'o', 'm', 'e', ' ', 'd', 'a', 't', 'a', '\0' };
    const auto bytes_transferred = libusb_control_transfer(
            device_handle,
            LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE, // bmRequestType
            0, // bRequest
            0, // wValue
            0, // wIndex
            &data[0],
            sizeof(data), // wLength
            1
        );
    if (bytes_transferred < 0) {
        print_libusb_error(static_cast<libusb_error>(bytes_transferred), "libusb_control_transfer");
        return false;
    } else {
        if (bytes_transferred != sizeof(data)) {
            printf("bytes_transferred %d, expected %lld\n", bytes_transferred, sizeof(data));
        }
        return true;
    }
}

bool control_transfer_in(libusb_device_handle *const device_handle) {
    unsigned char data[16] = { 0 };
    const auto bytes_transferred = libusb_control_transfer(
            device_handle,
            LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE, // bmRequestType
            0, // bRequest
            0, // wValue
            0, // wIndex
            &data[0],
            sizeof(data), // wLength
            1
        );
    if (bytes_transferred < 0) {
        print_libusb_error(static_cast<libusb_error>(bytes_transferred), "libusb_control_transfer");
        return false;
    } else {
        return true;
    }
}

bool bulk_transfer_in(libusb_device_handle *const device_handle) {
    assert(epbulk_in != invalid_ep_address);

    unsigned char data[64] = { 0 };
    int transferred = 0;
    const auto error = libusb_bulk_transfer(
            device_handle,
            epbulk_in,
            &data[0],
            sizeof(data),
            &transferred,
            1
        );
    if (error < 0) {
        print_libusb_error(static_cast<libusb_error>(error), "libusb_bulk_transfer");
        return false;
    } else {
        for (auto i = 0u; i < sizeof(data); ++i) {
            printf("0x%02" PRIx8 " ", data[i]);
        }
        putchar('\n');
        return true;
    }
}

void do_somthing_with_device(libusb_device_handle *const device_handle) {
    assert(device_handle);

    if (!check_configuration_value(device_handle)) return;

    // Control endpoint, i.e. endpoint 0, available regardless of whether or not interface is claimed
    if (!control_transfer_out(device_handle)) return;
    if (!control_transfer_in(device_handle)) return;

    if (!claim_interface(device_handle)) return;

    if (!bulk_transfer_in(device_handle)) return;

    if (!release_interface(device_handle)) return;
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
