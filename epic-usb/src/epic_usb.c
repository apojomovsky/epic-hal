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

/**
 * @brief Move queued TX bytes into the IN endpoint buffer and send.
 *
 * Sends at most one EP2 packet per call when the peripheral is
 * configured, the endpoint is not halted or busy, and the ring holds
 * data; otherwise returns without sending.
 */
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

/**
 * @brief Move one OUT-endpoint packet into the RX ring.
 *
 * Drains the OUT endpoint when the peripheral is configured, the
 * endpoint is not halted, and it holds data; bytes are dropped on ring
 * overflow. The endpoint is re-armed on return.
 */
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

/**
 * @brief Initialize the USB peripheral and start enumeration.
 *
 * Clears all ring and connection state, then hands control to
 * M-Stack's usb_init(). See epic_usb.h for the full contract.
 */
void epic_usb_init(void)
{
    g_tx_head = g_tx_tail = g_tx_count = 0u;
    g_rx_head = g_rx_tail = g_rx_count = 0u;
    g_dtr = false;
    usb_init();
}

/**
 * @brief Pump the USB stack.
 *
 * Services M-Stack, then drains one TX and one RX packet. See
 * epic_usb.h for the full contract.
 */
void epic_usb_service(void)
{
    usb_service();
    epic_usb_drain_tx();
    epic_usb_drain_rx();
}

/**
 * @brief Enqueue len bytes for transmission.
 *
 * Blocks (servicing internally) while the TX ring is full so the whole
 * buffer is enqueued before returning. See epic_usb.h for the full
 * contract.
 *
 * @param data bytes to transmit
 * @param len number of bytes to enqueue
 * @return the number of bytes enqueued (len unless len is 0)
 */
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

/**
 * @brief Pull up to max received bytes from the RX ring.
 *
 * Non-blocking. See epic_usb.h for the full contract.
 *
 * @param buf destination buffer
 * @param max maximum number of bytes to read
 * @return the number of bytes actually read (0 if nothing received)
 */
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

/**
 * @brief Report the number of bytes available to read from the RX ring.
 *
 * @return the number of received bytes buffered
 */
size_t epic_usb_available(void)
{
    return (size_t)g_rx_count;
}

/**
 * @brief Block until every enqueued TX byte has been transmitted.
 *
 * Services the stack internally until the TX ring has drained and the IN
 * endpoint is no longer busy. See epic_usb.h for the full contract.
 */
void epic_usb_flush(void)
{
    while (g_tx_count > 0u || usb_in_endpoint_busy(EPIC_USB_DATA_EP)) {
        epic_usb_service();
    }
}

/**
 * @brief Report whether the host has the CDC port open.
 *
 * @return true when DTR is asserted, false otherwise
 */
bool epic_usb_connected(void)
{
    return g_dtr;
}

/**
 * @brief Handle the SET_CONFIGURATION callback from M-Stack.
 *
 * Part of the M-Stack callbacks (named in usb_config.h). Every one must
 * exist even if unused: M-Stack calls them unconditionally. Only the DTR
 * (set_control_line_state) and unknown_setup_request (routes CDC class
 * requests) callbacks do anything; the rest are typed no-ops.
 *
 * @param configuration the configuration value being set
 */
void epic_usb_set_configuration_cb(uint8_t configuration)
{
    (void)configuration;
}

/**
 * @brief Handle the GET_DEVICE_STATUS callback from M-Stack.
 *
 * Typed no-op returning the default device status.
 *
 * @return 0x0000 (no remote wakeup, not self-powered)
 */
uint16_t epic_usb_get_device_status_cb(void)
{
    return 0x0000;
}

/**
 * @brief Handle the ENDPOINT_HALT callback from M-Stack.
 *
 * Typed no-op: halt state is managed by M-Stack itself.
 *
 * @param endpoint the endpoint whose halt state changed
 * @param halted true when the endpoint is being halted
 */
void epic_usb_endpoint_halt_cb(uint8_t endpoint, bool halted)
{
    (void)endpoint;
    (void)halted;
}

/**
 * @brief Handle the SET_INTERFACE callback from M-Stack.
 *
 * Always accepts the alternate setting.
 *
 * @param interface the interface being configured
 * @param alt_setting the alternate setting being selected
 * @return 0 (accepted)
 */
int8_t epic_usb_set_interface_cb(uint8_t interface, uint8_t alt_setting)
{
    (void)interface;
    (void)alt_setting;
    return 0;
}

/**
 * @brief Handle the GET_INTERFACE callback from M-Stack.
 *
 * @param interface the interface being queried
 * @return 0 (only alternate setting 0 exists)
 */
int8_t epic_usb_get_interface_cb(uint8_t interface)
{
    (void)interface;
    return 0;
}

/**
 * @brief Handle the OUT_TRANSACTION callback from M-Stack.
 *
 * Typed no-op: OUT data is drained in epic_usb_drain_rx() from the
 * service loop.
 *
 * @param endpoint the endpoint that received data
 */
void epic_usb_out_transaction_cb(uint8_t endpoint)
{
    (void)endpoint;
}

/**
 * @brief Handle the IN_TRANSACTION_COMPLETE callback from M-Stack.
 *
 * Typed no-op: TX completion is polled in epic_usb_service().
 *
 * @param endpoint the endpoint that finished transmitting
 */
void epic_usb_in_transaction_complete_cb(uint8_t endpoint)
{
    (void)endpoint;
}

/**
 * @brief Handle unknown setup requests from M-Stack.
 *
 * Routes CDC class requests to the M-Stack CDC driver.
 *
 * @param setup the parsed setup packet
 * @return the CDC driver's result code (0 on success)
 */
int8_t epic_usb_unknown_setup_request_cb(const struct setup_packet *setup)
{
    return process_cdc_setup_request(setup);
}

/**
 * @brief Handle unknown get-descriptor requests from M-Stack.
 *
 * Typed no-op: all descriptors are supplied statically.
 *
 * @param pkt the parsed setup packet
 * @param descriptor out-pointer for the descriptor
 * @return -1 (request not handled, stall)
 */
int16_t epic_usb_unknown_get_descriptor_cb(const struct setup_packet *pkt,
                                           const void **descriptor)
{
    (void)pkt;
    (void)descriptor;
    return -1;
}

/**
 * @brief Handle the START_OF_FRAME callback from M-Stack.
 *
 * Typed no-op: the module is polled, not SOF-driven.
 */
void epic_usb_start_of_frame_cb(void)
{
}

/**
 * @brief Handle the USB_RESET callback from M-Stack.
 *
 * Clears the DTR flag. The host clears DTR before a reset, but not
 * always before the next enumeration completes; force it false here so
 * epic_usb_connected() never reports stale state across a
 * reset/replug.
 */
void epic_usb_reset_cb(void)
{
    /* The host clears DTR before a reset, but not always before the next
     * enumeration completes; force it false here so epic_usb_connected()
     * never reports stale state across a reset/replug. */
    g_dtr = false;
}

/**
 * @brief Handle the CDC SEND_ENCAPSULATED_COMMAND request.
 *
 * Typed no-op: no abstract control management is implemented.
 *
 * @param interface the CDC interface index
 * @param length the encapsulated command length
 * @return -1 (not supported)
 */
int8_t epic_usb_cdc_send_encapsulated_command_cb(uint8_t interface, uint16_t length)
{
    (void)interface;
    (void)length;
    return -1;
}

/**
 * @brief Handle the CDC GET_ENCAPSULATED_RESPONSE request.
 *
 * Typed no-op: no abstract control management is implemented.
 *
 * @param interface the CDC interface index
 * @param length the response buffer length
 * @param report out-pointer for the response data
 * @param callback out-pointer for an EP0 data-stage callback
 * @param context out-pointer for the callback context
 * @return -1 (not supported)
 */
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

/**
 * @brief Handle the CDC SET_COMM_FEATURE request.
 *
 * Typed no-op: no communication features are implemented.
 *
 * @param interface the CDC interface index
 * @param idle_setting the requested idle setting
 * @param data_multiplexed_state the requested multiplexed state
 * @return -1 (not supported)
 */
int8_t epic_usb_cdc_set_comm_feature_cb(uint8_t interface, bool idle_setting,
                                        bool data_multiplexed_state)
{
    (void)interface;
    (void)idle_setting;
    (void)data_multiplexed_state;
    return -1;
}

/**
 * @brief Handle the CDC CLEAR_COMM_FEATURE request.
 *
 * Typed no-op: no communication features are implemented.
 *
 * @param interface the CDC interface index
 * @param idle_setting the idle setting to clear
 * @param data_multiplexed_state the multiplexed state to clear
 * @return -1 (not supported)
 */
int8_t epic_usb_cdc_clear_comm_feature_cb(uint8_t interface, bool idle_setting,
                                          bool data_multiplexed_state)
{
    (void)interface;
    (void)idle_setting;
    (void)data_multiplexed_state;
    return -1;
}

/**
 * @brief Handle the CDC GET_COMM_FEATURE request.
 *
 * Typed no-op: no communication features are implemented.
 *
 * @param interface the CDC interface index
 * @param idle_setting out-pointer for the idle setting
 * @param data_multiplexed_state out-pointer for the multiplexed state
 * @return -1 (not supported)
 */
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

/**
 * @brief Handle the CDC SET_LINE_CODING request.
 *
 * Stores the line coding (baud, format, parity, data bits) for
 * epic_usb_cdc_get_line_coding_cb().
 *
 * @param interface the CDC interface index
 * @param coding the line coding to store
 * @return 0 (accepted)
 */
int8_t epic_usb_cdc_set_line_coding_cb(uint8_t interface,
                                       const struct cdc_line_coding *coding)
{
    (void)interface;
    g_line_coding = *coding;
    return 0;
}

/**
 * @brief Handle the CDC GET_LINE_CODING request.
 *
 * @param interface the CDC interface index
 * @param coding out-pointer for the current line coding
 * @return 0 (success)
 */
int8_t epic_usb_cdc_get_line_coding_cb(uint8_t interface,
                                       struct cdc_line_coding *coding)
{
    (void)interface;
    *coding = g_line_coding;
    return 0;
}

/**
 * @brief Handle the CDC SET_CONTROL_LINE_STATE request.
 *
 * Records the DTR bit, which drives epic_usb_connected().
 *
 * @param interface the CDC interface index
 * @param dtr the DTR bit from the request
 * @param rts the RTS bit from the request (ignored)
 * @return 0 (accepted)
 */
int8_t epic_usb_cdc_set_control_line_state_cb(uint8_t interface, bool dtr, bool rts)
{
    (void)interface;
    (void)rts;
    g_dtr = dtr;
    return 0;
}

/**
 * @brief Handle the CDC SEND_BREAK request.
 *
 * Typed no-op: break signaling is not implemented.
 *
 * @param interface the CDC interface index
 * @param duration the break duration in 0.5 ms units (ignored)
 * @return 0 (accepted)
 */
int8_t epic_usb_cdc_send_break_cb(uint8_t interface, uint16_t duration)
{
    (void)interface;
    (void)duration;
    return 0;
}
