#include "evk-usb-device-hal.h"

#if !defined(DEVICE_USBDEVICE)

#include "usb-device.h"

#include <platform/mbed_assert.h>
#include <targets/TARGET_STM/TARGET_STM32F7/STM32Cube_FW/STM32F7xx_HAL_Driver/stm32f7xx_hal.h>

#include <algorithm>
#include <array>
#include <numeric>

namespace evk_usb_device_hal
{

namespace
{

constexpr uint8_t lsb(const uint16_t word);
constexpr uint8_t msb(const uint16_t word);

// 9.1 USB Device States describes the various states.
// I'm hoping we are only interested in a subset of states of the states described in the spec.
enum class device_state_t {
    default_,
    addressed,
    configured
};

// From Table 9-2. Format of Setup Data...
enum class recipient_t: uint8_t {
    device = 0x00,
    interface = 0x01,
    endpoint = 0x02,
    other = 0x03
};

enum class type_t: uint8_t {
    standard = 0x00,
    class_ = 0x20,
    vendor = 0x40
};

enum class direction_t: uint8_t {
    host_to_device = 0x00,
    device_to_host = 0x80
};

enum bmRequestType_masks {
    data_transfer_direction_mask = 0x80,
    type_mask = 0x60,
    recipient_mask = 0x1f
};

// From Table 9-4. Standard Request Codes...
enum class standard_request_codes: uint8_t {
    get_status = 0,
    clear_feature = 1,
    set_feature = 3,
    set_address = 5,
    get_descriptor = 6,
    set_descriptor = 7,
    get_configuration = 8,
    set_configuration = 9,
    get_interface = 10,
    set_interface = 11,
    synch_frame = 12
};

// From Table 9-5. Descriptor Types...
enum class descriptor_t: uint8_t {
    device = 1,
    configuration = 2,
    string = 3,
    interface = 4,
    endpoint = 5,
    device_qualifier = 6,
    other_speed_configuration = 7,
    interface_power = 8
};

// From Table 9-6. Standard Feature Selectors...
enum class standard_feature_selector_t: uint16_t {
    endpoint_halt = 0,
    device_remote_wakeup = 1,
    test_mode = 2
};

// From Table 9-10. Standard Configuration Descriptor...
enum configuration_attributes {
    configuration_attributes_reserved = 0x80,
    configuration_attributes_self_powered = 0x60,
    configuration_attributes_remote_wakeup = 0x40
};

// From Table 9-2. Format of Setup Data...
struct setup_data {
    struct request_type {
        direction_t direction;
        type_t type;
        recipient_t recipient;
    } bmRequestType;
    uint8_t bRequest;
    uint16_t wValue;
    uint16_t wIndex;
    uint16_t wLength;
};

// From 9.2.6.6 Speed Dependent Descriptors...
const uint16_t usb_version_2_0 = 0x0200;

enum string_index {
    langid = 0,
    manufacturer = 1,
    product = 2,
    serial_number = 3
};

const uint8_t ep0_out_ep_addr = 0x00;
const uint8_t ep0_in_ep_addr = 0x80;
const uint8_t ep1_out_ep_addr = 0x01;
const uint8_t ep1_in_ep_addr = 0x81;

const size_t device_descriptor_length = 18;
uint8_t device_descriptor[device_descriptor_length] = {
    // device descriptor, USB spec 9.6.1
    device_descriptor_length,  // bLength
    static_cast<uint8_t>(descriptor_t::device),  // bDescriptorType
    lsb(usb_version_2_0),    // bcdUSB
    msb(usb_version_2_0),
    0x00,                   // bDeviceClass
    0x00,                   // bDeviceSubClass
    0x00,                   // bDeviceprotocol
    USB_OTG_MAX_EP0_SIZE,   // bMaxPacketSize0
    lsb(0x1f00),            // idVendor
    msb(0x1f00),
    lsb(0x2012),            // idProduct
    msb(0x2012),
    lsb(0x0001),            // bcdDevice
    msb(0x0001),
    string_index::manufacturer,  // iManufacturer
    string_index::product,  // iProduct
    string_index::serial_number,  // iSerialNumber
    1                       // bNumConfigurations
};

// From 9.6.7 String...
const size_t langid_string_descriptor_length = 4;
uint8_t langid_string_descriptor[langid_string_descriptor_length] = {
    langid_string_descriptor_length,  // bLength
    static_cast<uint8_t>(descriptor_t::string),  // bDescriptorType
    0x09, 0x04,             // bString Lang ID - 0x0409 - English (United States)
};

const size_t manufacturer_string_descriptor_length = 8;
uint8_t manufacturer_string_descriptor[manufacturer_string_descriptor_length] = {
    manufacturer_string_descriptor_length,  // bLength
    static_cast<uint8_t>(descriptor_t::string),  // bDescriptorType
    'M', 0, 'B', 0, 'r', 0  // bString
};

const size_t product_string_descriptor_length = 8;
uint8_t product_string_descriptor[product_string_descriptor_length] = {
    product_string_descriptor_length,  // bLength
    static_cast<uint8_t>(descriptor_t::string),  // bDescriptorType
    'E', 0, 'V', 0, 'K', 0  // bString
};

const size_t serial_number_string_descriptor_length = 10;
uint8_t serial_number_string_descriptor[serial_number_string_descriptor_length] = {
    serial_number_string_descriptor_length,  // bLength
    static_cast<uint8_t>(descriptor_t::string),  // bDescriptorType
    '0', 0, '0', 0, '0', 0, '1', 0  // bString
};

const uint8_t default_configuration = 1;
const size_t configuration_descriptor_length = 9;
const size_t interface_descriptor_length = 9;
const size_t endpoint_descriptor_length = 7;
const size_t total_configuration_descriptor_length = configuration_descriptor_length + interface_descriptor_length + 2 * endpoint_descriptor_length;
uint8_t configuration_descriptor[total_configuration_descriptor_length] = {
    // configuration descriptor, USB spec 9.6.3
    configuration_descriptor_length,  // bLength
    static_cast<uint8_t>(descriptor_t::configuration),  // bDescriptorType
    lsb(total_configuration_descriptor_length),  // wTotalLength
    msb(total_configuration_descriptor_length),
    1,                      // bNumInterfaces
    default_configuration,  // bConfigurationValue
    0,                      // iConfiguration
    configuration_attributes_reserved,  // bmAttributes
    50,                     // bMaxPower

    // interface descriptor, USB spec 9.6.5
    interface_descriptor_length, // bLength
    static_cast<uint8_t>(descriptor_t::interface),  // bDescriptorType
    0,                      // bInterfaceNumber
    0,                      // bAlternateSetting
    2,                      // bNumEndpoints
    0xff,                   // bInterfaceClass
    0xff,                   // bInterfaceSubClass
    0xff,                   // bInterfaceProtocol
    0,                      // iInterface

    // endpoint descriptor, USB spec 9.6.6
    endpoint_descriptor_length, // bLength
    static_cast<uint8_t>(descriptor_t::endpoint),  // bDescriptorType
    ep1_in_ep_addr,         // bEndpointAddress - EP1 bulk in
    EP_TYPE_BULK,           // bmAttributes
    lsb(USB_OTG_HS_MAX_PACKET_SIZE),  // wMaxPacketSize
    msb(USB_OTG_HS_MAX_PACKET_SIZE),
    1,                      // bInterval

    endpoint_descriptor_length, // bLength
    static_cast<uint8_t>(descriptor_t::endpoint),  // bDescriptorType
    ep1_out_ep_addr,        // bEndpointAddress - EP1 bulk out
    EP_TYPE_BULK,           // bmAttributes
    lsb(USB_OTG_HS_MAX_PACKET_SIZE),  // wMaxPacketSize
    msb(USB_OTG_HS_MAX_PACKET_SIZE),
    1,                      // bInterval
};

PCD_HandleTypeDef hpcd = {
    .Instance = USB_OTG_HS,
    .Init = {
        .dev_endpoints = 9,  // STM32F72x has 1 bidirectional control endpoint0 and 8 HS endpoints configurable to support bulk, interrupt or isochronous transfers
        .Host_channels = 0,  // STM32F72x has 16 HS host channels but setting to 0 as not using host mode
        .speed = PCD_SPEED_HIGH,
        // Disable embedded DMA initially although need to find out how to use DMA.
        // HAL_PCD_Init sets this to DISABLE depending on the value of OTG_CID i.e. it checks if this is a HS device
        .dma_enable = ENABLE,
        .ep0_mps = USB_OTG_MAX_EP0_SIZE,
        .phy_itface = USB_OTG_HS_EMBEDDED_PHY,
        .Sof_enable = DISABLE,
        .low_power_enable = DISABLE,
        .lpm_enable = DISABLE,
        .battery_charging_enable = DISABLE,
        .vbus_sensing_enable = ENABLE,
        .use_dedicated_ep1 = DISABLE,
        .use_external_vbus = DISABLE
    },
    .USB_Address = 0,  // HAL_PCD_Init set this to 0
    // HAL_PCD_Init initialises the IN_ep and OUT_ep upto the number specified in in dev_endpoints.
    .IN_ep = { 0 },
    .OUT_ep = { 0 },
    .Lock = HAL_UNLOCKED,
    .State = HAL_PCD_STATE_RESET,
    .ErrorCode = 0,
    .Setup = { 0 },
    .LPM_State = LPM_L0,
    .BESL = 0,
    .lpm_active = DISABLE,
    .pData = nullptr,
};

device_state_t device_state = device_state_t::default_;

// usb-host does a /test/ control out request with a payload of "some data"
// AFAICT, the DMA engine writes a whole packet to the destination buffer even if it is a short transfer.
std::array<uint8_t, USB_OTG_MAX_EP0_SIZE> vendor_request_receive_buffer{ 0x00 };
bool vendor_request_receive_buffer_ready = false;
const auto vendor_request_receive_expected = std::array<uint8_t, 10>{"some data"};

std::array<uint8_t, usb_device::bulk_transfer_length> ep1_transmit_buffer;
std::array<uint8_t, usb_device::bulk_transfer_length> ep1_receive_buffer;

constexpr uint8_t lsb(const uint16_t word) {
    // Not sure that the explicit mask is necessary.
    return word & 0xff;
}

constexpr uint8_t msb(const uint16_t word) {
    return (word & 0xff00) >> 8;
}

setup_data decode_setup_packet(const uint32_t setup[]) {
    const uint8_t bmRequestType = setup[0] & 0xff;
    setup_data setup_data = {
        .bmRequestType = {
            .direction = static_cast<direction_t>(bmRequestType & data_transfer_direction_mask),
            .type = static_cast<type_t>(bmRequestType & type_mask),
            .recipient = static_cast<recipient_t>(bmRequestType & recipient_mask)
        },
        .bRequest = static_cast<uint8_t>((setup[0] & 0xff00) >> 8),
        .wValue = static_cast<uint16_t>((setup[0] & 0xffff0000) >> 16),
        .wIndex = static_cast<uint16_t>(setup[1] & 0x0000ffff),
        .wLength = static_cast<uint16_t>((setup[1] & 0xffff0000) >> 16)
    };
    return setup_data;
}

descriptor_t decode_descriptor_type(const uint16_t wValue) {
    return static_cast<descriptor_t>((wValue & 0xff00) >> 8);
}

uint16_t decode_string_index(const uint16_t wValue) {
    return wValue & 0x00ff;
}

void get_string_descriptor(const uint16_t string_index, uint8_t *&pBuf, uint32_t &len) {
    switch (string_index) {
        case string_index::langid:
            pBuf = &langid_string_descriptor[0];
            len = langid_string_descriptor_length;
            break;
        case string_index::manufacturer:
            pBuf = &manufacturer_string_descriptor[0];
            len = manufacturer_string_descriptor_length;
            break;
        case string_index::product:
            pBuf = &product_string_descriptor[0];
            len = product_string_descriptor_length;
            break;
        case string_index::serial_number:
            pBuf = &serial_number_string_descriptor[0];
            len = serial_number_string_descriptor_length;
            break;
        default:
            MBED_ASSERT(false);
    }
}

void get_descriptor(PCD_HandleTypeDef *const hpcd, const setup_data &setup_data) {
    uint8_t *pBuf = nullptr;
    uint32_t len = 0;

    // I thought I had read in the spec that the request can have zero length in which case
    // there's no point decoding the request because nothing can be returned.
    // Although I haven't seen this happen yet.
    if (setup_data.wLength > 0) {
        const auto descriptor_type = decode_descriptor_type(setup_data.wValue);
        switch(descriptor_type) {
            case descriptor_t::device:
                pBuf = &device_descriptor[0];
                len = device_descriptor_length;
                break;
            case descriptor_t::configuration:
                pBuf = &configuration_descriptor[0];
                len = std::min<uint32_t>(setup_data.wLength, total_configuration_descriptor_length);
                break;
            case descriptor_t::string: {
                get_string_descriptor(decode_string_index(setup_data.wValue), pBuf, len);
                break;
            }
            case descriptor_t::interface:
            case descriptor_t::endpoint:
            case descriptor_t::device_qualifier:
            case descriptor_t::other_speed_configuration:
            case descriptor_t::interface_power:
                MBED_ASSERT(false);
        }
    }

    if (len <= setup_data.wLength) {
        HAL_PCD_EP_Transmit(hpcd, 0x00, pBuf, len);
    } else {
        // I haven't accounted for the request length being shorter than the descriptor because I haven't seen it happen.
        // If/when it does happen it is necessary to implement a mechanism for sending the data in multiple packets.
        MBED_ASSERT(false);
    }
}

void device_get_status(PCD_HandleTypeDef *const hpcd, const uint16_t wLength) {
    // A static buffer is required for the response to a 'Get Status' request.
    // I think the '0x1' indicates 'self-powered' but I'm not sure we are 'self-powered'
    // and I'm not sure this is the correct bit position.
    static uint8_t status[2] = { 0x01, 0x00 };

    // Should solicit USB error response but this will do for now.
    MBED_ASSERT(wLength == 2);
    // Not being in one of these states should really solicit a USB error but the implementation doesn't support this yet.
    MBED_ASSERT(device_state == device_state_t::default_ || device_state == device_state_t::addressed || device_state == device_state_t::configured);

    HAL_PCD_EP_Transmit(hpcd, ep0_out_ep_addr, &status[0], 2);
}

void set_address(PCD_HandleTypeDef *const hpcd, const setup_data &setup_data) {
    MBED_ASSERT(setup_data.wIndex == 0);
    MBED_ASSERT(setup_data.wLength == 0);

    // Should really respond with a USB error but this will do for starters.
    MBED_ASSERT(device_state != device_state_t::configured);

    const uint8_t address = setup_data.wValue;
    MBED_ASSERT(address < 128);

    HAL_PCD_SetAddress(hpcd, address);
    // From 9.2.6.3 Set Address Processing...
    //     In the case of the SetAddress() request, the Status stage successfully completes when the device sends
    //     the zero-length Status packet or when the device sees the ACK in response to the Status stage data packet.
    // Hence send "zero-length Status packet".
    HAL_PCD_EP_Transmit(hpcd, ep0_out_ep_addr, nullptr, 0);

    device_state = address != 0 ? device_state_t::addressed : device_state_t::default_;
}

void set_configuration(PCD_HandleTypeDef *const hpcd, const setup_data &setup_data) {
    const auto configuration = setup_data.wValue;
    if (configuration == default_configuration) {
        // Open the bulk in endpoint and prepare to transmit data when host requests it.
        HAL_PCD_EP_Open(hpcd, ep1_in_ep_addr, USB_OTG_HS_MAX_PACKET_SIZE, EP_TYPE_BULK);
        HAL_PCD_EP_Transmit(hpcd, ep1_in_ep_addr, ep1_transmit_buffer.data(), ep1_transmit_buffer.size());

        // Open the bulk out endpoint and prepare to receive data when the host sends it.
        HAL_PCD_EP_Open(hpcd, ep1_out_ep_addr, USB_OTG_HS_MAX_PACKET_SIZE, EP_TYPE_BULK);
        HAL_PCD_EP_Receive(hpcd, ep1_out_ep_addr, ep1_receive_buffer.data(), ep1_receive_buffer.size());

        // Indicate configuration successfully set...
        HAL_PCD_EP_Transmit(hpcd, ep0_out_ep_addr, nullptr, 0);

        device_state = device_state_t::configured;
    } else if (configuration == 0) {
        HAL_PCD_EP_Transmit(hpcd, ep0_out_ep_addr, nullptr, 0);
        device_state = device_state_t::addressed;
    } else {
        // The configuration can be 0 in which case the device should enter the 'Address state'.
        // See 9.4.7 Set Configuration.
        // But, I haven't seen this happen.
        MBED_ASSERT(false);
    }
}

void standard_device_request(PCD_HandleTypeDef *const hpcd, const setup_data &setup_data) {
    switch (static_cast<standard_request_codes>(setup_data.bRequest)) {
        case standard_request_codes::get_status:
            device_get_status(hpcd, setup_data.wLength);
            break;
        case standard_request_codes::set_address:
            set_address(hpcd, setup_data);
            break;
        case standard_request_codes::get_descriptor:
            get_descriptor(hpcd, setup_data);
            break;
        case standard_request_codes::set_configuration:
            set_configuration(hpcd, setup_data);
            break;
        case standard_request_codes::clear_feature:
        case standard_request_codes::set_feature:
        case standard_request_codes::set_descriptor:
        case standard_request_codes::get_configuration:
        case standard_request_codes::get_interface:
        case standard_request_codes::set_interface:
        case standard_request_codes::synch_frame:
            MBED_ASSERT(false);
    }
}

void vendor_device_request(PCD_HandleTypeDef *const hpcd, const setup_data &setup_data) {
    switch (setup_data.bRequest) {
        case 0:
            if (setup_data.bmRequestType.direction == direction_t::host_to_device) {
                vendor_request_receive_buffer.fill(0);

                const auto len = std::min<uint32_t>(setup_data.wLength, vendor_request_receive_expected.size());
                HAL_PCD_EP_Receive(hpcd, ep0_in_ep_addr, vendor_request_receive_buffer.data(), len);
                HAL_PCD_EP_Transmit(hpcd, ep0_out_ep_addr, nullptr, 0);

                vendor_request_receive_buffer_ready = true;
            } else {
                static std::array<uint8_t, 13> send_request_data{ "send request" };
                const auto len = std::min<uint32_t>(setup_data.wLength, send_request_data.size());
                HAL_PCD_EP_Transmit(hpcd, ep0_out_ep_addr, send_request_data.data(), len);
            }
            break;
        default:
            MBED_ASSERT(false);
    }
}

void device_request(PCD_HandleTypeDef *const hpcd, const setup_data &setup_data) {
    const auto bmRequestType = setup_data.bmRequestType;
    switch (bmRequestType.type) {
        case type_t::standard:
            standard_device_request(hpcd, setup_data);
            break;
        case type_t::class_:
            MBED_ASSERT(false);
        case type_t::vendor:
            vendor_device_request(hpcd, setup_data);
            break;
    }
}

void endpoint_clear(PCD_HandleTypeDef *const hpcd, const setup_data &setup_data) {
    if (device_state == device_state_t::configured) {
        const standard_feature_selector_t feature_selector_value = static_cast<standard_feature_selector_t>(setup_data.wValue);
        if (feature_selector_value == standard_feature_selector_t::endpoint_halt) {
            const uint16_t ep_addr = setup_data.wIndex;
            if ((ep_addr & 0x7f) != 0x00) {
                HAL_PCD_EP_ClrStall(hpcd, ep_addr);
            }
            HAL_PCD_EP_Transmit(hpcd, ep0_out_ep_addr, nullptr, 0);
        }
    }
}

void standard_endpoint_request(PCD_HandleTypeDef *const hpcd, const setup_data &setup_data) {
    switch (static_cast<standard_request_codes>(setup_data.bRequest)) {
        case standard_request_codes::clear_feature:
            endpoint_clear(hpcd, setup_data);
            break;
        case standard_request_codes::get_status:
        case standard_request_codes::set_address:
        case standard_request_codes::get_descriptor:
        case standard_request_codes::set_configuration:
        case standard_request_codes::set_feature:
        case standard_request_codes::set_descriptor:
        case standard_request_codes::get_configuration:
        case standard_request_codes::get_interface:
        case standard_request_codes::set_interface:
        case standard_request_codes::synch_frame:
            MBED_ASSERT(false);
    }
}

void endpoint_request(PCD_HandleTypeDef *const hpcd, const setup_data &setup_data) {
    const auto bmRequestType = setup_data.bmRequestType;
    switch (bmRequestType.type) {
        case type_t::standard:
            standard_endpoint_request(hpcd, setup_data);
            break;
        case type_t::class_:
        case type_t::vendor:
            MBED_ASSERT(false);
    }
}

void setup_stage_callback(PCD_HandleTypeDef *const hpcd) {
    const auto setup_data = decode_setup_packet(hpcd->Setup);
    const auto bmRequestType = setup_data.bmRequestType;

    switch (bmRequestType.recipient) {
        case recipient_t::device:
            device_request(hpcd, setup_data);
            break;
        case recipient_t::endpoint:
            endpoint_request(hpcd, setup_data);
            break;
        case recipient_t::interface:
        case recipient_t::other:
            MBED_ASSERT(false);
    }
}

}

void init() {
    std::iota(std::begin(ep1_transmit_buffer), std::end(ep1_transmit_buffer), 1);

    // 'STM32Cube_FW_F7_V1.16.0/Projects/STM32F723E-Discovery/Applications/USB_Device/HID_Standalone/Src/usbd_conf.c'
    // uses 'HAL_PCD_MspInit' to do some of the configuration. 'HAL_PCD_MspInit' is called from 'HAL_PCD_Init' and it is
    // intended that the application will use it for low level configuration such as clocks and GPIOs.
    // I'm not sure what value it adds and 'USBPhyHw::init' doesn't use.
    // I'm just going to do all the initialisation here, e.g. the clocks are enabled below.
    // 'HAL_PCD_MspInit' in 'usbd_conf.c' initialises the GPIOs for USB FS but not for USB HS.
    // It appears the USB HS GPIOs don't need to be configured for their alternate functions.
    // I don't understand this and perhaps it will need revisiting.

    __HAL_RCC_OTGPHYC_CLK_ENABLE();
    __HAL_RCC_USB_OTG_HS_CLK_ENABLE();
    __HAL_RCC_USB_OTG_HS_ULPI_CLK_ENABLE();

    MBED_UNUSED const HAL_StatusTypeDef ret = HAL_PCD_Init(&hpcd);
    MBED_ASSERT(ret == HAL_OK);

    // The STM32F72x has 4 Kbytes of dedicated RAM for the USB data FIFOs in HS mode.
    // FIFO size is set as 32-bit words i.e. 0x200 words is 2048 bytes or half the dedicated RAM.
    // From 32.11.3 FIFO RAM allocation/Device Mode of the reference manual:
    //     10 locations must be reserved in the receive FIFO to receive SETUP packets on control endpoint.
    //     One location is to be allocated for Global OUT NAK.
    // By location I think it means 'word'.
    // The reference manual doesn't say where these reserved locations are but it appears from
    // 'USBPhyHw::init' and 'STM32Cube_FW_F7_V1.16.0/Projects/STM32F723E-Discovery/Applications/USB_Device/HID_Standalone/Src/usbd_conf.c'
    // that not all the words should be allocated here.
    // 'USBPhyHw::init' is a poor example because it thinks there is only '1.25 kbytes' of RAM available.

    const auto hs_usb_data_fifo_ram_size_bytes = 4096;
    const auto hs_usb_data_fifo_ram_size = hs_usb_data_fifo_ram_size_bytes / 4;  // I.e. 0x400 words

#undef MAXIMISE_TXFIFO_SIZE  // For now I'm going to stick with FIFO configuration from the examples, perhaps if/when the endpoint configuration gets more complicated this will need revisiting.
#if defined(MAXIMISE_TXFIFO_SIZE)
    // From 32.11.3 FIFO RAM allocation...
    //     10 locations must be reserved in the receive FIFO to receive SETUP packets on control endpoint. The core does not use these locations, which are reserved for SETUP packets, to write any other data.
    // This isn't shown the equation in the RM and it's possible/probably I don't understand.
    // But I found that without this magic number the OUT transaction timed out.
    // It turns out I'm not the only person to struggle with this and the documentation comes in for some critism, see
    //     https://github.com/IntergatedCircuits/USBDevice4Cube/issues/1#issuecomment-439653127
    const auto reserved_locations = 10;

    //     Device RxFIFO = (5 * number of control endpoints + 8) + ((largest USB packet used / 4) + 1 for status information) + (2 * number of OUT endpoints) + 1 for Global NAK
    //                   = (5 * 2 + 8) + ((512 / 4) + 1) + (2 * 2) + 1
    //                   = 152 = 0x98
    const auto num_ctrl_eps = 2;  // I.e. EP0 In and OUT
    const auto largest_packet = std::max(USB_OTG_MAX_EP0_SIZE, USB_OTG_HS_MAX_PACKET_SIZE);
    const auto num_out_eps = 2;  // I.e. EP0 OUT and EP1 OUT
    const auto rx_fifo_size = reserved_locations + (5 * num_ctrl_eps + 8) + ((largest_packet / 4) + 1) + (2 * num_out_eps) + 1;

    //     Transmit FIFO RAM allocation: the minimum RAM space required for each IN endpoint Transmit FIFO is the maximum packet size for that particular IN endpoint.
    //     More space allocated in the transmit IN endpoint FIFO results in better performance on the USB.
    // In other words, 1 packet should be enough but I've gone for 2 to be safe. My test only does 1 EP0 IN transfer.
    const auto ep0_in_mps = USB_OTG_MAX_EP0_SIZE;
    const auto ep0_tx_fifo_size_bytes = ep0_in_mps * 2;
    const auto ep0_tx_fifo_size = ep0_tx_fifo_size_bytes / 4;

    // I can't ratify this with the documentation but the examples leave 12 words unallocated. I wonder if there's some confusion about the '10 reserved words'.
    // I'm going to do the same, just in case...
    const auto magic_free_unallocated_space = 12;

    // Allocate the remaining data FIFO RAM to the EP1 Tx FIFO
    const auto ep1_tx_fifo_size = hs_usb_data_fifo_ram_size - rx_fifo_size - ep0_tx_fifo_size - magic_free_unallocated_space;

    HAL_PCDEx_SetRxFiFo(&hpcd, rx_fifo_size);  // Rx FIFO size must be set first
    HAL_PCDEx_SetTxFiFo(&hpcd, 0, ep0_tx_fifo_size);  // Tx FIFOs for IN endpoints must be set in order
    HAL_PCDEx_SetTxFiFo(&hpcd, 1, ep1_tx_fifo_size);
#else
    // I've copied these numbers from 'usbd_conf.c'.
    // I think the rational is something like:
    //     Packets of varying sizes can be received so allocate an arbitrarily large RX FIFO i.e. half the available space.
    //     Control packets on EP0 have a max size of 64 bytes so I think 0x80 means there's room for 8 TX control packets in the FIFO.
    //     TX packets on EP1 could be 512 bytes so 0x174 words (1488 bytes) means there's room for nearly 3 packets in the FIFO.
    // 'usbd_conf.c' leaves 12 words unallocated i.e. 0x200 + 0x80 + 0x174 = 0x400 - 12. I've never understood why...
    //     HAL_PCDEx_SetRxFiFo(&hpcd, 0x200);
    //     HAL_PCDEx_SetTxFiFo(&hpcd, 0, 0x80);
    //     HAL_PCDEx_SetTxFiFo(&hpcd, 1, 0x174);
    // I found a different example, 'STM32Cube_FW_F7_V1.16.0/Projects/STM32746G-Discovery/Applications/USB_Device/DFU_Standalone/Src/usbd_conf.c', that allocates all of the data RAM...
    //      HAL_PCDEx_SetRxFiFo(&hpcd, 0x200);
    //      HAL_PCDEx_SetTxFiFo(&hpcd, 0, 0x200);
    // I tried allocating all the data RAM and it seemed to work although it is possible that it only works because the Rx FIFO has plenty of head room.
    // This is important because of the DMA. The DMA is word aligned and I think is copies whole packets. A Tx FIFO of 0x174 words, 1488 bytes, is not a whole number of packets
    // and I think this causes the DMA problem. The DMA didn't work when the Tx FIFO was 0x174 words and did when it was 0x100 words.
    // I've been around a few houses and now think the Tx FIFO has to be an integer number of packets, i.e. it worked when I tried 0x100 and 0x200 and not with 0x180.
    // I suspect this still isn't the end of the story.
    const auto rx_fifo_size = 0x200;
    const auto ep0_tx_fifo_size = 0x80;
    MBED_UNUSED const auto available_for_ep1_tx_fifo = hs_usb_data_fifo_ram_size - rx_fifo_size - ep0_tx_fifo_size;
    const auto ep1_tx_fifo_size = (USB_OTG_HS_MAX_PACKET_SIZE * 2) / 4;
    static_assert(ep1_tx_fifo_size <= available_for_ep1_tx_fifo, "No enough space available for the EP1 Tx FIFO");

    HAL_PCDEx_SetRxFiFo(&hpcd, rx_fifo_size);  // Rx FIFO size must be set first
    HAL_PCDEx_SetTxFiFo(&hpcd, 0, ep0_tx_fifo_size);  // Tx FIFOs for IN endpoints must be set in order
    HAL_PCDEx_SetTxFiFo(&hpcd, 1, ep1_tx_fifo_size);
#endif

    // Set USB HS interrupt to the lowest priority
    HAL_NVIC_SetPriority(OTG_HS_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(OTG_HS_IRQn);

    HAL_PCD_Start(&hpcd);
}

}

// Replace /weak/ definition provided by 'startup_stm32f723xx.s' so needs to be in the global namespace.
extern "C" void OTG_HS_IRQHandler() {
    HAL_PCD_IRQHandler(&evk_usb_device_hal::hpcd);
}

// This is the first callback called after calling 'HAL_PCD_Start' and connecting the device.
// This is a significantly simplified version of 'HAL_PCD_ResetCallback' from 'usbd_conf.c'.
// A USB device must always have EP0 open for In and OUT transactions.
void HAL_PCD_ResetCallback(PCD_HandleTypeDef *const hpcd) {
    evk_usb_device_hal::device_state = evk_usb_device_hal::device_state_t::default_;

    HAL_PCD_EP_Open(hpcd, evk_usb_device_hal::ep0_out_ep_addr, USB_OTG_MAX_EP0_SIZE, EP_TYPE_CTRL);

    HAL_PCD_EP_Open(hpcd, evk_usb_device_hal::ep0_in_ep_addr, USB_OTG_MAX_EP0_SIZE, EP_TYPE_CTRL);
}

void HAL_PCD_DataOutStageCallback(PCD_HandleTypeDef *hpcd, uint8_t epnum) {
    if (epnum == 0) {
        if (evk_usb_device_hal::vendor_request_receive_buffer_ready) {
            const auto count = hpcd->OUT_ep[epnum].xfer_count;

            if (count < evk_usb_device_hal::vendor_request_receive_expected.size()) {
                MBED_ASSERT(false);
            }

            const auto equal = std::equal(begin(evk_usb_device_hal::vendor_request_receive_expected), end(evk_usb_device_hal::vendor_request_receive_expected), begin(evk_usb_device_hal::vendor_request_receive_buffer));
            if (!equal) {
                MBED_ASSERT(false);
            }
            evk_usb_device_hal::vendor_request_receive_buffer_ready = false;
        }
    } else if (epnum == 1) {
        // ep1_receive_buffer contains data from the bulk transfer so clear ready for the next transfer...
        evk_usb_device_hal::ep1_receive_buffer.fill(0);
        // Prepare for another transfer...
        HAL_PCD_EP_Receive(hpcd, evk_usb_device_hal::ep1_out_ep_addr, evk_usb_device_hal::ep1_receive_buffer.data(), evk_usb_device_hal::ep1_receive_buffer.size());
    }
}

void HAL_PCD_DataInStageCallback(PCD_HandleTypeDef *hpcd, uint8_t epnum) {
    // Simplified version of 'USBD_LL_DataInStage' in
    // 'STM32Cube_FW_F7_V1.16.0/Projects/STM32F723E-Discovery/Applications/USB_Device/HID_Standalone/Src/usbd_core.c'.
    if (epnum == 0) {
        // I believe it is necessary for the device to receive the ack sent by the host in the status stage of the sequence.
        // See Figure 8-37. Control Read and Write Sequences.
        // I don't understand what happens if the host doesn't send the ack or sends a nak or something.
        HAL_PCD_EP_Receive(hpcd, evk_usb_device_hal::ep0_out_ep_addr, nullptr, 0);
    } else if (epnum == 1) {
        // Prepare for another transfer...
        HAL_PCD_EP_Transmit(hpcd, evk_usb_device_hal::ep1_in_ep_addr, evk_usb_device_hal::ep1_transmit_buffer.data(), evk_usb_device_hal::ep1_transmit_buffer.size());
    }
}

// 'HAL_PCD_IRQHandler' decodes setup packets, puts the payload in 'hpcd->Setup'
// and calls 'HAL_PCD_SetupStageCallback'.
// The packet is described in the USB Spec 9.3 USB Device Requests.
void HAL_PCD_SetupStageCallback(PCD_HandleTypeDef *hpcd)
{
    evk_usb_device_hal::setup_stage_callback(hpcd);
}

#endif
