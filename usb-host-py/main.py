r"""usb-host-py - Simple host for a USB device.
"""

import usb.core
import usb.util

_INVALID_EP_ADDRESS = 0xff
_epbulk_in_address = _INVALID_EP_ADDRESS
_epbulk_in_mps = 0
_epbulk_out_address = _INVALID_EP_ADDRESS
_epbulk_out_mps = 0

def _print_device_list():
    print(usb.core.show_devices())

def _get_end_point_addresses(device):
    global _epbulk_in_address, _epbulk_in_mps, _epbulk_out_address, _epbulk_out_mps

    configuration = device[0]
    if configuration.bNumInterfaces != 1:
        print("unexpected number of interfaces {}".format(configuration.bNumInterfaces))
        return None

    interface = configuration[(0, 0)]
    if interface.bAlternateSetting != 0:
        print("unexpected number of alternate settings {}".format(interface.bAlternateSetting))
        return None
    if interface.bNumEndpoints != 2:
        print("unexpected number of endpoint {}".format(interface.bNumEndpoints))
        return None

    for endpoint in interface:
        print(
            "bEndpointAddress 0x{:x} bmAttributes 0x{:x} wMaxPacketSize 0x{:x}"
            .format(endpoint.bEndpointAddress, endpoint.bmAttributes, endpoint.wMaxPacketSize)
            )

        is_in = usb.util.endpoint_direction(endpoint.bEndpointAddress) == usb.util.ENDPOINT_IN
        is_bulk = usb.util.endpoint_type(endpoint.bmAttributes) == usb.util.ENDPOINT_TYPE_BULK
        if is_in and is_bulk:
            if _epbulk_in_address != _INVALID_EP_ADDRESS:
                print("Found multiple bulk IN endpoints")
                return None
            _epbulk_in_address = endpoint.bEndpointAddress
            _epbulk_in_mps = endpoint.wMaxPacketSize

        is_out = usb.util.endpoint_direction(endpoint.bEndpointAddress) == usb.util.ENDPOINT_IN
        if is_out and is_bulk:
            if _epbulk_out_address != _INVALID_EP_ADDRESS:
                print("Found multiple bulk IN endpoints")
                return None
            _epbulk_out_address = endpoint.bEndpointAddress
            _epbulk_out_mps = endpoint.wMaxPacketSize

    return device

def _open_device(id_vendor, id_product):
    device = usb.core.find(idVendor=id_vendor, idProduct=id_product)
    if device is not None:
        print("found device with idVendor 0x{:x} idProduct 0x{:x}".format(id_vendor, id_product))
        device.set_configuration()
        return _get_end_point_addresses(device)

    print(
        "failed to find device with idVendor 0x{:x} idProduct 0x{:x}"
        .format(id_vendor, id_product)
        )

    return None

def _check_configuration_value(device):
    configuration = device.get_active_configuration()

    if configuration.bConfigurationValue == 1:
        return True

    print(
        "'libusb_get_configuration' succeeded, configuration value is {}"
        .format(configuration.bConfigurationValue)
        )

    return False

def _control_transfer_out(device):
    data = "some data\0"
    bytes_transferred = device.ctrl_transfer(
        bmRequestType
            = usb.util.CTRL_OUT
            | usb.util.CTRL_TYPE_VENDOR
            | usb.util.CTRL_RECIPIENT_DEVICE,
        bRequest=0,
        wValue=0,
        wIndex=0,
        data_or_wLength=data
        )
    print("bytes_transferred {}".format(bytes_transferred))
    if bytes_transferred != len(data):
        print("bytes_transferred {}, expected {}".format(bytes_transferred, len(data)))
        return False

    return True

def _control_transfer_in(device):
    data = device.ctrl_transfer(
        bmRequestType
            = usb.util.CTRL_IN
            | usb.util.CTRL_TYPE_VENDOR
            | usb.util.CTRL_RECIPIENT_DEVICE,
        bRequest=0,
        wValue=0,
        wIndex=0,
        data_or_wLength=13
        )
    if len(data) != 13:
        return False

    return True

def _do_somthing_with_device(device):
    assert device is not None

    if not _check_configuration_value(device):
        return

    # Control endpoint, i.e. endpoint 0, available regardless of whether or not interface is claimed
    if not _control_transfer_out(device):
        return
    if not _control_transfer_in(device):
        return

def _main():
    print("usb-host")

    devices = list(usb.core.find(find_all=True))
    print("number_of_devices {}".format(len(devices)))
    _print_device_list()

    device = _open_device(0x1f00, 0x2012)

    _do_somthing_with_device(device)

if __name__ == "__main__":
    _main()
