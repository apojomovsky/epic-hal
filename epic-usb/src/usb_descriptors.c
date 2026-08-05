/*
 * USB descriptors for epic-usb, adapted from M-Stack's cdc_acm demo
 * (same shape, required by the CDC-ACM spec; epic-usb-specific strings).
 * VID/PID 0xA0A0/0x0004 is M-Stack's placeholder pair, fine for bench
 * bring-up only, swap for a real pid.codes allocation before
 * redistributing a device carrying it.
 */

#include "usb_config.h"
#include "usb.h"
#include "usb_ch9.h"
#include "usb_cdc.h"

struct configuration_1_packet {
	struct configuration_descriptor  config;
	struct interface_association_descriptor iad;

	/* CDC Class (Communications) Interface */
	struct interface_descriptor      cdc_class_interface;
	struct cdc_functional_descriptor_header cdc_func_header;
	struct cdc_acm_functional_descriptor cdc_acm;
	struct cdc_union_functional_descriptor cdc_union;
	struct endpoint_descriptor       cdc_ep;

	/* CDC Data Interface */
	struct interface_descriptor      cdc_data_interface;
	struct endpoint_descriptor       data_ep_in;
	struct endpoint_descriptor       data_ep_out;
};

const struct device_descriptor this_device_descriptor =
{
	sizeof(struct device_descriptor), // bLength
	DESC_DEVICE, // bDescriptorType
	0x0200, // USB 2.0
	DEVICE_CLASS_MISC, // Device class
	0x02, /* Device Subclass -- "USB Interface Association Descriptor
	         Device Class Code and Use Model" */
	0x01, // Protocol, see document referenced above
	EP_0_LEN, // bMaxPacketSize0
	0xA0A0, // Vendor -- M-Stack placeholder, see file header comment
	0x0004, // Product -- ditto
	0x0001, // device release (1.0)
	1, // Manufacturer (string index)
	2, // Product (string index)
	3, // Serial (string index)
	NUMBER_OF_CONFIGURATIONS // NumConfigurations
};

static const struct configuration_1_packet configuration_1 =
{
	{
	// struct configuration_descriptor
	sizeof(struct configuration_descriptor),
	DESC_CONFIGURATION,
	sizeof(configuration_1), // wTotalLength
	2, // bNumInterfaces
	1, // bConfigurationValue
	0, // iConfiguration
	0b10000000,
	100/2, // 100mA
	},

	/* Interface Association Descriptor */
	{
	sizeof(struct interface_association_descriptor),
	DESC_INTERFACE_ASSOCIATION,
	0, /* bFirstInterface */
	2, /* bInterfaceCount */
	CDC_COMMUNICATION_INTERFACE_CLASS,
	CDC_COMMUNICATION_INTERFACE_CLASS_ACM_SUBCLASS,
	0, /* bFunctionProtocol */
	0, /* iFunction */
	},

	/* CDC Class Interface */
	{
	sizeof(struct interface_descriptor),
	DESC_INTERFACE,
	0x0, // InterfaceNumber
	0x0, // AlternateSetting
	0x1, // bNumEndpoints
	CDC_COMMUNICATION_INTERFACE_CLASS,
	CDC_COMMUNICATION_INTERFACE_CLASS_ACM_SUBCLASS,
	0x00, // bInterfaceProtocol
	0x00, // iInterface
	},

	/* CDC Functional Descriptor Header */
	{
	sizeof(struct cdc_functional_descriptor_header),
	DESC_CS_INTERFACE,
	CDC_FUNCTIONAL_DESCRIPTOR_SUBTYPE_HEADER,
	0x0110, /* bcdCDC */
	},

	/* CDC ACM Functional Descriptor */
	{
	sizeof(struct cdc_acm_functional_descriptor),
	DESC_CS_INTERFACE,
	CDC_FUNCTIONAL_DESCRIPTOR_SUBTYPE_ACM,
	/* Keep in sync with the callbacks actually implemented in
	 * epic_usb.c: epic_usb_cdc_set/get_line_coding_cb and
	 * epic_usb_cdc_send_break_cb are real, so both capability bits
	 * apply. */
	CDC_ACM_CAPABILITY_LINE_CODINGS | CDC_ACM_CAPABILITY_SEND_BREAK,
	},

	/* CDC Union Functional Descriptor */
	{
	sizeof(struct cdc_union_functional_descriptor),
	DESC_CS_INTERFACE,
	CDC_FUNCTIONAL_DESCRIPTOR_SUBTYPE_UNION,
	0, /* bMasterInterface */
	1, /* bSlaveInterface0 */
	},

	/* CDC ACM Notification Endpoint (Endpoint 1 IN), required by the
	 * CDC-ACM interface descriptor; epic_usb.c never sends serial-state
	 * notifications on it. */
	{
	sizeof(struct endpoint_descriptor),
	DESC_ENDPOINT,
	0x01 | 0x80, // endpoint #1, IN
	EP_INTERRUPT,
	EP_1_IN_LEN,
	1, // bInterval (ms)
	},

	/* CDC Data Interface */
	{
	sizeof(struct interface_descriptor),
	DESC_INTERFACE,
	0x1, // InterfaceNumber
	0x0, // AlternateSetting
	0x2, // bNumEndpoints
	CDC_DATA_INTERFACE_CLASS,
	0, // bInterfaceSubclass
	CDC_DATA_INTERFACE_CLASS_PROTOCOL_NONE,
	0x00, // iInterface
	},

	/* CDC Data IN Endpoint -- epic_usb_read() source */
	{
	sizeof(struct endpoint_descriptor),
	DESC_ENDPOINT,
	0x02 | 0x80, // endpoint #2, IN
	EP_BULK,
	EP_2_IN_LEN,
	1,
	},

	/* CDC Data OUT Endpoint -- epic_usb_write() destination */
	{
	sizeof(struct endpoint_descriptor),
	DESC_ENDPOINT,
	0x02, // endpoint #2, OUT
	EP_BULK,
	EP_2_OUT_LEN,
	1,
	},
};

/* String descriptors. UTF-16, not NULL-terminated. */

static const struct {uint8_t bLength; uint8_t bDescriptorType; uint16_t lang;} str00 = {
	sizeof(str00), DESC_STRING, 0x0409 /* US English */
};

static const struct {uint8_t bLength; uint8_t bDescriptorType; uint16_t chars[16];} vendor_string = {
	sizeof(vendor_string), DESC_STRING,
	{'p','i','c','8','-','u','s','b',' ','p','r','o','j','e','c','t'}
};

static const struct {uint8_t bLength; uint8_t bDescriptorType; uint16_t chars[8];} product_string = {
	sizeof(product_string), DESC_STRING,
	{'p','i','c','8','-','u','s','b'}
};

static const struct {uint8_t bLength; uint8_t bDescriptorType; uint16_t chars[29];} dev_serial_string = {
	sizeof(dev_serial_string), DESC_STRING,
	{'D','E','V',' ','B','U','I','L','D',' ','-',' ','N','O','T',' ',
	 'F','O','R',' ','S','H','I','P','P','I','N','G','.'}
};

int16_t usb_application_get_string(uint8_t string_number, const void **ptr)
{
	if (string_number == 0) {
		*ptr = &str00;
		return sizeof(str00);
	}
	else if (string_number == 1) {
		*ptr = &vendor_string;
		return sizeof(vendor_string);
	}
	else if (string_number == 2) {
		*ptr = &product_string;
		return sizeof(product_string);
	}
	else if (string_number == 3) {
		/* Placeholder, not a real per-unit serial number: a real
		 * deployment needs a genuine one (e.g. from EEPROM) so a host
		 * doesn't confuse two boards sharing this VID/PID. */
		*ptr = &dev_serial_string;
		return sizeof(dev_serial_string);
	}

	return -1;
}

const struct configuration_descriptor *usb_application_config_descs[] =
{
	(struct configuration_descriptor*) &configuration_1,
};
STATIC_SIZE_CHECK_EQUAL(USB_ARRAYLEN(USB_CONFIG_DESCRIPTOR_MAP), NUMBER_OF_CONFIGURATIONS);
STATIC_SIZE_CHECK_EQUAL(sizeof(USB_DEVICE_DESCRIPTOR), 18);
