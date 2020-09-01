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
        print("bEndpointAddress 0x{:x} bmAttributes 0x{:x} wMaxPacketSize 0x{:x}".format(endpoint.bEndpointAddress, endpoint.bmAttributes, endpoint.wMaxPacketSize))

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
    else:
        print("failed to find device with idVendor 0x{:x} idProduct 0x{:x}".format(id_vendor, id_product))

    return device

def _main():
    print("usb-host")

    devices = list(usb.core.find(find_all=True))
    print("number_of_devices {}".format(len(devices)))
    _print_device_list()

    device = _open_device(0x1f00, 0x2012)

if __name__ == "__main__":
    _main()
