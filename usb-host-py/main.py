import usb.core

def print_device_list():
    print(usb.core.show_devices())

def main():
    print("usb-host")

    devices = list(usb.core.find(find_all=True))
    print("number_of_devices {}".format(len(devices)))
    print_device_list()

if __name__ == "__main__":
    main()
