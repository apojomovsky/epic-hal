/*
 * Real-target implementation: ring buffers over M-Stack's raw
 * endpoint-buffer API (which has no CDC read/write of its own), plus
 * the M-Stack callbacks usb_config.h points at. One EP2 packet drained
 * per service() call, RX drops on ring overflow. Host build uses
 * epic_usb_host_stub.c instead.
 */

#include "epic_usb.h"

#include "usb.h"
#include "usb_ch9.h"
#include "usb_cdc.h"

#define EPIC_USB_DATA_EP 2u
#define MASK             (EPIC_USB_RING_SZ - 1u)

static uint8_t g_tx_buf[EPIC_USB_RING_SZ];
static uint8_t g_tx_head, g_tx_tail, g_tx_count;
static uint8_t g_rx_buf[EPIC_USB_RING_SZ];
static uint8_t g_rx_head, g_rx_tail, g_rx_count;
static bool    g_dtr;

static void epic_usb_drain_tx(void)
{
    if (!usb_is_configured() ||
        usb_in_endpoint_halted(EPIC_USB_DATA_EP) ||
        usb_in_endpoint_busy(EPIC_USB_DATA_EP) ||
        g_tx_count == 0u) {
        return;
    }

    unsigned char *buf = usb_get_in_buffer(EPIC_USB_DATA_EP);
    size_t n = 0;
    while (n < EP_2_IN_LEN && g_tx_count > 0u) {
        buf[n++] = g_tx_buf[g_tx_tail];
        g_tx_tail = (uint8_t)((g_tx_tail + 1u) & MASK);
        g_tx_count--;
    }
    usb_send_in_buffer(EPIC_USB_DATA_EP, n);
}

static void epic_usb_drain_rx(void)
{
    if (!usb_is_configured() ||
        usb_out_endpoint_halted(EPIC_USB_DATA_EP) ||
        !usb_out_endpoint_has_data(EPIC_USB_DATA_EP)) {
        return;
    }

    const unsigned char *out_buf;
    uint8_t len = usb_get_out_buffer(EPIC_USB_DATA_EP, &out_buf);
    for (uint8_t i = 0; i < len; i++) {
        if (g_rx_count < EPIC_USB_RING_SZ) {    /* drop on overflow */
            g_rx_buf[g_rx_head] = out_buf[i];
            g_rx_head = (uint8_t)((g_rx_head + 1u) & MASK);
            g_rx_count++;
        }
    }
    usb_arm_out_endpoint(EPIC_USB_DATA_EP);
}

void epic_usb_init(void)
{
    g_tx_head = g_tx_tail = g_tx_count = 0u;
    g_rx_head = g_rx_tail = g_rx_count = 0u;
    g_dtr = false;
    usb_init();
}

void epic_usb_service(void)
{
    usb_service();
    epic_usb_drain_tx();
    epic_usb_drain_rx();
}

size_t epic_usb_write(const uint8_t *data, size_t len)
{
    for (size_t i = 0; i < len; i++) {
        while (g_tx_count >= EPIC_USB_RING_SZ) {
            epic_usb_service();    /* ring full: drain as we block */
        }
        g_tx_buf[g_tx_head] = data[i];
        g_tx_head = (uint8_t)((g_tx_head + 1u) & MASK);
        g_tx_count++;
    }
    epic_usb_service();            /* kick a send now, not on the next poll */
    return len;
}

size_t epic_usb_read(uint8_t *buf, size_t max)
{
    size_t n = 0;
    while (n < max && g_rx_count > 0u) {
        buf[n++] = g_rx_buf[g_rx_tail];
        g_rx_tail = (uint8_t)((g_rx_tail + 1u) & MASK);
        g_rx_count--;
    }
    return n;
}

size_t epic_usb_available(void)
{
    return (size_t)g_rx_count;
}

void epic_usb_flush(void)
{
    while (g_tx_count > 0u || usb_in_endpoint_busy(EPIC_USB_DATA_EP)) {
        epic_usb_service();
    }
}

bool epic_usb_connected(void)
{
    return g_dtr;
}

/* M-Stack callbacks (named in usb_config.h). Every one must exist even
 * if unused: M-Stack calls them unconditionally. Only the DTR
 * (set_control_line_state) and unknown_setup_request (routes CDC class
 * requests) callbacks do anything; the rest are typed no-ops. */

void epic_usb_set_configuration_cb(uint8_t configuration)
{
    (void)configuration;
}

uint16_t epic_usb_get_device_status_cb(void)
{
    return 0x0000;
}

void epic_usb_endpoint_halt_cb(uint8_t endpoint, bool halted)
{
    (void)endpoint;
    (void)halted;
}

int8_t epic_usb_set_interface_cb(uint8_t interface, uint8_t alt_setting)
{
    (void)interface;
    (void)alt_setting;
    return 0;
}

int8_t epic_usb_get_interface_cb(uint8_t interface)
{
    (void)interface;
    return 0;
}

void epic_usb_out_transaction_cb(uint8_t endpoint)
{
    (void)endpoint;
}

void epic_usb_in_transaction_complete_cb(uint8_t endpoint)
{
    (void)endpoint;
}

int8_t epic_usb_unknown_setup_request_cb(const struct setup_packet *setup)
{
    return process_cdc_setup_request(setup);
}

int16_t epic_usb_unknown_get_descriptor_cb(const struct setup_packet *pkt,
                                           const void **descriptor)
{
    (void)pkt;
    (void)descriptor;
    return -1;
}

void epic_usb_start_of_frame_cb(void)
{
}

void epic_usb_reset_cb(void)
{
    /* The host clears DTR before a reset, but not always before the next
     * enumeration completes; force it false here so epic_usb_connected()
     * never reports stale state across a reset/replug. */
    g_dtr = false;
}

int8_t epic_usb_cdc_send_encapsulated_command_cb(uint8_t interface, uint16_t length)
{
    (void)interface;
    (void)length;
    return -1;
}

int16_t epic_usb_cdc_get_encapsulated_response_cb(uint8_t interface, uint16_t length,
                                                  const void **report,
                                                  usb_ep0_data_stage_callback *callback,
                                                  void **context)
{
    (void)interface;
    (void)length;
    (void)report;
    (void)callback;
    (void)context;
    return -1;
}

int8_t epic_usb_cdc_set_comm_feature_cb(uint8_t interface, bool idle_setting,
                                        bool data_multiplexed_state)
{
    (void)interface;
    (void)idle_setting;
    (void)data_multiplexed_state;
    return -1;
}

int8_t epic_usb_cdc_clear_comm_feature_cb(uint8_t interface, bool idle_setting,
                                          bool data_multiplexed_state)
{
    (void)interface;
    (void)idle_setting;
    (void)data_multiplexed_state;
    return -1;
}

int8_t epic_usb_cdc_get_comm_feature_cb(uint8_t interface, bool *idle_setting,
                                        bool *data_multiplexed_state)
{
    (void)interface;
    (void)idle_setting;
    (void)data_multiplexed_state;
    return -1;
}

static struct cdc_line_coding g_line_coding =
{
    115200,
    CDC_CHAR_FORMAT_1_STOP_BIT,
    CDC_PARITY_NONE,
    8,
};

int8_t epic_usb_cdc_set_line_coding_cb(uint8_t interface,
                                       const struct cdc_line_coding *coding)
{
    (void)interface;
    g_line_coding = *coding;
    return 0;
}

int8_t epic_usb_cdc_get_line_coding_cb(uint8_t interface,
                                       struct cdc_line_coding *coding)
{
    (void)interface;
    *coding = g_line_coding;
    return 0;
}

int8_t epic_usb_cdc_set_control_line_state_cb(uint8_t interface, bool dtr, bool rts)
{
    (void)interface;
    (void)rts;
    g_dtr = dtr;
    return 0;
}

int8_t epic_usb_cdc_send_break_cb(uint8_t interface, uint16_t duration)
{
    (void)interface;
    (void)duration;
    return 0;
}
