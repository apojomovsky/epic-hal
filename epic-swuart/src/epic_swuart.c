/* Bit-banged full-duplex UART on CCP hardware capture/compare timing.
 * See docs/ARCHITECTURE.md for the shared-tick design and docs/API.md
 * for per-function semantics. */
#include "epic_swuart.h"

/* TX state machine states. TX_DATA covers all 8 data bits,
 * distinguished by tx_bit_index: a per-bit enum needs a duplicate
 * case for bit 7 that fires one spurious transition on stale
 * shifted-out data, so counting makes "8 bits done" explicit. */
enum {
    TX_IDLE = 0,
    TX_DATA,
    TX_STOP,
};

/* RX state machine states. */
enum {
    RX_IDLE = 0,
    RX_CONFIRM_START,
    RX_DATA0, RX_DATA1, RX_DATA2, RX_DATA3,
    RX_DATA4, RX_DATA5, RX_DATA6, RX_DATA7,
    RX_STOP,
};

/* Forward declaration: real definition (RX state machine) is below,
 * after g_chan_a/g_cycles_per_bit/SWUART_CCP_RX exist; EPIC_SWUART_Init
 * needs the symbol earlier when it builds ccp_rx's EventCallback. */
static void on_rx_event_a(void);
#if EPIC_SWUART_MAX_CHANNELS >= 2
static void on_rx_event_b(void);
#endif

/* Cycles of lead time between EPIC_SWUART_Write() arming the start bit's
 * compare deadline and that deadline actually landing: must be large
 * enough that EPIC_CCP_SetCompare/SetMode land before Timer1 reaches the
 * armed value, confirmed on real PIC16F877A hardware to need 120
 * cycles, not the original 40-cycle guess. */
#define SWUART_LEAD_CYCLES 120u

/* One bit period in instruction cycles: round(FOSC_HZ / 4 / baud).
 * Timer1 prescaler stays 1:1 (unchanged from v1) so this is directly
 * the Timer1 counter delta for one bit. */
static uint16_t compute_cycles_per_bit(uint32_t fosc_hz, uint32_t baud)
{
    uint32_t cycles = (fosc_hz / 4u + baud / 2u) / baud;
    if (cycles > 65535u) cycles = 65535u;
    if (cycles < 1u) cycles = 1u;
    return (uint16_t)cycles;
}

static uint16_t g_cycles_per_bit = 0u;
static EPIC_SWUART_HandleTypeDef *g_chan_a = NULL;
static TIMER1_HandleTypeDef s_timer1 = TIMER1_HANDLE_DEFAULT;

#define SWUART_CCP_RX CCP_INSTANCE_1
#define SWUART_CCP_TX CCP_INSTANCE_2

/* Second channel, PIC16F193X only: the only family with five CCP
 * modules, enough for two full RX+TX pairs (PIC16F87XA/PIC18Fxx5x have
 * exactly two CCP modules total, already spent on channel A).
 * CCP3 = RX (RB5), CCP4 = TX (RD1), DS41364D pin diagram (40-pin PDIP,
 * PIC16F1937), neither remapped by pic16f193x_ccp.c (no APFCON writes
 * anywhere in that driver), so the POR default pin applies. */
#if EPIC_SWUART_MAX_CHANNELS >= 2
static EPIC_SWUART_HandleTypeDef *g_chan_b = NULL;
#define SWUART_CCP_RX_B CCP_INSTANCE_3
#define SWUART_CCP_TX_B CCP_INSTANCE_4
#endif

/* Arms the mode for the *next* compare match, not the one that just
 * fired: the hardware already toggled the pin per what was armed
 * ahead, so this only decides the deadline it is about to write.
 * Traced against 'A' = 0x41, LSB first: Write() arms CLEAR (start),
 * then SET/CLEAR per data bit, SET (stop), landing in TX_IDLE exactly
 * at the stop bit's deadline. */
static void tx_compare_event(EPIC_SWUART_HandleTypeDef *h, CCP_InstanceTypeDef tx_inst)
{
    CCP_ModeTypeDef next_mode;

    switch (h->tx_state) {
    case TX_IDLE:
        if (h->tx_count == 0u) {
            EPIC_CCP_SetMode(tx_inst, CCP_MODE_OFF);
            return;
        }
        h->tx_shift = h->tx_ring[h->tx_tail];
        h->tx_tail = (uint8_t)((h->tx_tail + 1u) & (EPIC_SWUART_RING_SZ - 1u));
        h->tx_count--;
        h->tx_bit_index = 0u;
        next_mode = CCP_MODE_COMPARE_CLEAR;
        h->tx_state = TX_DATA;
        break;
    case TX_DATA:
        next_mode = (h->tx_shift & 0x01u) ? CCP_MODE_COMPARE_SET : CCP_MODE_COMPARE_CLEAR;
        h->tx_shift >>= 1;
        h->tx_bit_index++;
        if (h->tx_bit_index >= 8u) h->tx_state = TX_STOP;
        break;
    case TX_STOP:
    default:
        next_mode = CCP_MODE_COMPARE_SET;
        h->tx_state = TX_IDLE;
        break;
    }

    h->tx_deadline = (uint16_t)(h->tx_deadline + g_cycles_per_bit);
    EPIC_CCP_SetCompare(tx_inst, h->tx_deadline);
    EPIC_CCP_SetMode(tx_inst, next_mode);
}

static void on_tx_event_a(void) { tx_compare_event(g_chan_a, SWUART_CCP_TX); }
#if EPIC_SWUART_MAX_CHANNELS >= 2
static void on_tx_event_b(void) { tx_compare_event(g_chan_b, SWUART_CCP_TX_B); }
#endif

/* Test-only hooks (see test_swuart_tx.c): default-disabled. Undefined
 * unless a build explicitly opts in, so real-target builds (which never
 * touch epic-swuart/CMakeLists.txt) never compile this in. Host-sim
 * test executables that need it get EPIC_SWUART_TEST_HOOKS=1 from
 * epic-swuart/CMakeLists.txt, scoped to just those targets. */
#ifdef EPIC_SWUART_TEST_HOOKS
/* PIC_REG_CCP2CON: every family's own sfr.h defines this name at that
 * family's actual CCP2CON address (0x1D on PIC16F87XA, 0xFBA on
 * PIC18Fxx5x, 0x29A on PIC16F193X), reached transitively via
 * epic_hal.h. Previously hardcoded to PIC16F87XA's 0x1D with no family
 * guard (deferred finding from Task 4); only channel A's CCP2 is ever
 * read here, so this stays correct even on PIC16F193X where channel B
 * exists too. */
uint8_t swuart_test_last_tx_mode(void) { return (uint8_t)EPIC_REG8(PIC_REG_CCP2CON); }
uint16_t swuart_test_last_tx_compare(void) { return g_chan_a->tx_deadline; }
void swuart_test_fire_tx_event(void) { on_tx_event_a(); }
#if EPIC_SWUART_MAX_CHANNELS >= 2
/* Channel B's own TX hooks: PIC_REG_CCP4CON is channel B's real TX CCP
 * control register (0x31A on PIC16F193X, the only family this branch
 * ever compiles for), so a test reading this instead of CCP2CON can
 * tell whether a Write() on channel B actually armed channel B's own
 * hardware, not channel A's (see EPIC_SWUART_Write's dispatch fix). */
uint8_t swuart_test_last_tx_mode_b(void) { return (uint8_t)EPIC_REG8(PIC_REG_CCP4CON); }
uint16_t swuart_test_last_tx_compare_b(void) { return g_chan_b->tx_deadline; }
void swuart_test_fire_tx_event_b(void) { on_tx_event_b(); }
#endif
#endif

static void rx_push(EPIC_SWUART_HandleTypeDef *h, uint8_t byte)
{
    if (h->rx_count >= EPIC_SWUART_RING_SZ) {
        h->error_count++;
        return;
    }
    h->rx_ring[h->rx_head] = byte;
    h->rx_head = (uint8_t)((h->rx_head + 1u) & (EPIC_SWUART_RING_SZ - 1u));
    h->rx_count++;
}

/* Channel B (PIC16F193X only, CCP3) keeps the two-fire
 * capture-then-confirm sequence; only channel A on PIC16F87XA gets the
 * collapsed fast path (rx_capture_event_fast hardcodes CCP1's 87XA SFR
 * addresses; see EPIC_SWUART_HAS_RX_FAST_PATH). PIC18Fxx5x and
 * PIC16F193X channel A still use this generic path, so the guard is
 * "channel B exists OR no fast path exists". */
#if EPIC_SWUART_MAX_CHANNELS >= 2 || !EPIC_SWUART_HAS_RX_FAST_PATH
/* test_get_capture takes the RX CCP instance (not hardcoded to channel
 * A's) so the shared rx_capture_event body below reads the right
 * hardware capture register for whichever channel is firing; the test
 * double still returns one shared value regardless of instance, since
 * no existing test exercises both channels' RX side at once. */
#if EPIC_SWUART_TEST_HOOKS
static uint16_t g_test_capture_value = 0u;
void swuart_test_set_capture(uint16_t value) { g_test_capture_value = value; }
static uint16_t test_get_capture(CCP_InstanceTypeDef rx_inst) { (void)rx_inst; return g_test_capture_value; }
#else
static uint16_t test_get_capture(CCP_InstanceTypeDef rx_inst) { return EPIC_CCP_GetCapture(rx_inst); }
#endif

/* Shared RX capture/compare event body, parameterised by handle and CCP
 * instance the same way tx_compare_event is: on_rx_event_b below is a
 * thin wrapper over this. */
static void rx_capture_event(EPIC_SWUART_HandleTypeDef *h, CCP_InstanceTypeDef rx_inst)
{
    if (h->rx_state == RX_IDLE) {
        /* Capture-mode event: a start-bit falling edge just arrived,
         * hardware-timestamped, immune to how late this handler
         * actually runs. */
        uint16_t edge_time = test_get_capture(rx_inst);
        /* Half a bit period: the mid-start-bit deglitch confirm point,
         * NOT d0's sample; the RX_CONFIRM_START branch below adds one
         * more full bit period on top, landing d0 at edge_time + 1.5
         * cycles_per_bit. A single 1.5x hop would skip the deglitch:
         * at 1.5x post-edge the pin already reflects d0's value, so
         * noise starts would pass. */
        h->rx_deadline = (uint16_t)(edge_time + g_cycles_per_bit / 2u);
        h->rx_state = RX_CONFIRM_START;
        EPIC_CCP_SetCompare(rx_inst, h->rx_deadline);
        EPIC_CCP_SetMode(rx_inst, CCP_MODE_COMPARE_SOFT_IF);
        return;
    }

    if (h->rx_state == RX_CONFIRM_START) {
        if (EPIC_GPIO_ReadPin(h->rx_port, h->rx_pin) != GPIO_PIN_RESET) {
            h->rx_state = RX_IDLE; /* noise, not a real start bit */
            EPIC_CCP_SetMode(rx_inst, CCP_MODE_CAPTURE_FALLING);
            return;
        }
        h->rx_shift = 0u;
        h->rx_bit_index = 0u;
        h->rx_state = RX_DATA0;
        h->rx_deadline = (uint16_t)(h->rx_deadline + g_cycles_per_bit);
        EPIC_CCP_SetCompare(rx_inst, h->rx_deadline);
        return;
    }

    uint8_t sample = (EPIC_GPIO_ReadPin(h->rx_port, h->rx_pin) == GPIO_PIN_SET) ? 1u : 0u;

    if (h->rx_state == RX_STOP) {
        if (sample != 0u) {
            rx_push(h, h->rx_shift);
        } else {
            h->error_count++;
        }
        h->rx_state = RX_IDLE;
        EPIC_CCP_SetMode(rx_inst, CCP_MODE_CAPTURE_FALLING);
        return;
    }

    h->rx_shift = (uint8_t)((h->rx_shift >> 1) | (sample ? 0x80u : 0u));
    h->rx_bit_index++;
    h->rx_state = (h->rx_bit_index < 8u) ? (uint8_t)(RX_DATA0 + h->rx_bit_index) : RX_STOP;
    h->rx_deadline = (uint16_t)(h->rx_deadline + g_cycles_per_bit);
    EPIC_CCP_SetCompare(rx_inst, h->rx_deadline);
}
#endif /* EPIC_SWUART_MAX_CHANNELS >= 2 || !EPIC_SWUART_HAS_RX_FAST_PATH */

/* Direct SFR addresses for PIC16F87XA's CCP1 (channel A's RX capture),
 * confirmed against pic16f87xa_ccp.c's addrs[0] entry. Bypasses
 * EPIC_CCP_GetCapture's atomic retry-loop (unnecessary: a second real
 * capture can't land for a full cycles_per_bit, ~521 cycles at 9600
 * baud) and the generic SetCompare/SetMode call overhead, for this one
 * hot path only. Hardcoded to CCP1: channel B (PIC16F193X, CCP3) keeps
 * the generic rx_capture_event, a disclosed scope limit (see
 * docs/ARCHITECTURE.md's "Known limitations"); porting the pattern to
 * other families/instances is a follow-up. */
#if EPIC_SWUART_HAS_RX_FAST_PATH
#define CCP1_CPRL_ADDR 0x15U
#define CCP1_CPRH_ADDR 0x16U
#define CCP1_CON_ADDR  0x17U

/* Edge-to-Timer1-read latency on real PIC16F87XA (AN555-style
 * _Cycle_Offset1 correction): 3 cycles interrupt response + 322 cycles
 * software latency, measured via an mdb probe on PIC16F877A, XC8 v3.10,
 * -O2. With it,
 * d0's sample deadline lands mid-d0 at 1.499 bit periods; with 0 it
 * landed inside d1's window, a guaranteed mis-sample. Do not re-derive
 * without a fresh probe: it describes this exact code path/toolchain. */
#define RX_CAPTURE_OVERHEAD_CYCLES 325u

#if EPIC_SWUART_TEST_HOOKS
/* Writes straight into the same two registers rx_capture_event_fast
 * itself reads, so an injected test value flows through the exact
 * production code path, not a separate indirection layer. */
void swuart_test_set_capture_fast(uint16_t value)
{
    EPIC_REG8(CCP1_CPRH_ADDR) = (uint8_t)(value >> 8);
    EPIC_REG8(CCP1_CPRL_ADDR) = (uint8_t)(value & 0xFFu);
}
#endif

static void rx_capture_event_fast(EPIC_SWUART_HandleTypeDef *h)
{
    if (h->rx_state != RX_IDLE) {
        /* Steady-state per-bit sampling, unchanged from v3's own
         * arithmetic: each event only needs to beat the *next* one by
         * a full cycles_per_bit, already proven to fit with real
         * margin (v3's TX-side measurement). */
        uint8_t sample = (EPIC_GPIO_ReadPin(h->rx_port, h->rx_pin) == GPIO_PIN_SET) ? 1u : 0u;

        if (h->rx_state == RX_STOP) {
            if (sample != 0u) {
                rx_push(h, h->rx_shift);
            } else {
                h->error_count++;
            }
            h->rx_state = RX_IDLE;
            EPIC_REG8(CCP1_CON_ADDR) = (uint8_t)CCP_MODE_CAPTURE_FALLING;
            return;
        }

        h->rx_shift = (uint8_t)((h->rx_shift >> 1) | (sample ? 0x80u : 0u));
        h->rx_bit_index++;
        h->rx_state = (h->rx_bit_index < 8u) ? (uint8_t)(RX_DATA0 + h->rx_bit_index) : RX_STOP;
        h->rx_deadline = (uint16_t)(h->rx_deadline + g_cycles_per_bit);
        EPIC_REG8(CCP1_CPRL_ADDR) = (uint8_t)(h->rx_deadline & 0xFFu);
        EPIC_REG8(CCP1_CPRH_ADDR) = (uint8_t)(h->rx_deadline >> 8);
        return;
    }

    /* RX_IDLE: a start-bit falling edge just latched into CCPR1.
     * Immediate, synchronous deglitch check, no second scheduled
     * event (deferring the check to a second event was the v3 timing
     * race this removes). */
    if (EPIC_GPIO_ReadPin(h->rx_port, h->rx_pin) != GPIO_PIN_RESET) {
        return; /* noise: pin already back high, stay in Capture mode */
    }

    h->rx_shift = 0u;
    h->rx_bit_index = 0u;
    h->rx_state = RX_DATA0;

    /* Relative reload: "now" is read after this handler's own real
     * latency has already elapsed, so the deadline can never be in
     * the past, unlike v3's edge_time + 0.5*cycles_per_bit scheme.
     * RX_CAPTURE_OVERHEAD_CYCLES corrects for that already-elapsed
     * latency so the arm still lands close to the intended 1.5-bit
     * mark relative to the real edge, not relative to "now". */
    uint16_t now = EPIC_TIMER1_ReadCounter();
    uint16_t target_offset = (uint16_t)(g_cycles_per_bit + g_cycles_per_bit / 2u);
    h->rx_deadline = (uint16_t)(now + target_offset - RX_CAPTURE_OVERHEAD_CYCLES);

    EPIC_REG8(CCP1_CPRL_ADDR) = (uint8_t)(h->rx_deadline & 0xFFu);
    EPIC_REG8(CCP1_CPRH_ADDR) = (uint8_t)(h->rx_deadline >> 8);
    EPIC_REG8(CCP1_CON_ADDR) = (uint8_t)CCP_MODE_COMPARE_SOFT_IF;
}
#endif /* EPIC_SWUART_HAS_RX_FAST_PATH */

/* Channel A's handler is the fast path on PIC16F87XA only; on
 * PIC18Fxx5x and PIC16F193X it stays on the generic rx_capture_event
 * (which reads/writes CCP1 through the family-correct EPIC_CCP_* SFR
 * accessors) until a follow-up ports the fast pattern. */
#if EPIC_SWUART_HAS_RX_FAST_PATH
static void on_rx_event_a(void) { rx_capture_event_fast(g_chan_a); }
#else
static void on_rx_event_a(void) { rx_capture_event(g_chan_a, SWUART_CCP_RX); }
#endif
#if EPIC_SWUART_MAX_CHANNELS >= 2
static void on_rx_event_b(void) { rx_capture_event(g_chan_b, SWUART_CCP_RX_B); }
#endif

#if EPIC_SWUART_TEST_HOOKS
void swuart_test_fire_rx_event(void) { on_rx_event_a(); }
#if EPIC_SWUART_MAX_CHANNELS >= 2
void swuart_test_fire_rx_event_b(void) { on_rx_event_b(); }
#endif
#endif

EPIC_StatusTypeDef EPIC_SWUART_Init(EPIC_SWUART_HandleTypeDef *h,
                                     GPIO_TypeDef tx_port, uint16_t tx_pin,
                                     GPIO_TypeDef rx_port, uint16_t rx_pin,
                                     uint32_t fosc_hz, uint32_t baud)
{
    if (!h) return EPIC_INVALID;

    /* Slot A, every family: CCP1 = RC2 (RX), CCP2 = RC1 (TX). Same port
     * and pin numbers on PIC16F87XA and PIC16F193X (checked against
     * both families' own datasheets, not assumed from the shared
     * macro names). */
    uint8_t slot_a_match = (tx_port == GPIOC && tx_pin == GPIO_PIN_1 &&
                             rx_port == GPIOC && rx_pin == GPIO_PIN_2) ? 1u : 0u;
    CCP_InstanceTypeDef rx_inst;
    CCP_InstanceTypeDef tx_inst;
    void (*rx_cb)(void);
    void (*tx_cb)(void);
#if EPIC_SWUART_MAX_CHANNELS >= 2
    uint8_t use_slot_b = 0u;
#endif

    if (slot_a_match && g_chan_a == NULL) {
        rx_inst = SWUART_CCP_RX; tx_inst = SWUART_CCP_TX;
        rx_cb = on_rx_event_a; tx_cb = on_tx_event_a;
    }
#if EPIC_SWUART_MAX_CHANNELS >= 2
    /* Slot B, PIC16F193X only (5 CCP modules, enough for a second
     * RX+TX pair): CCP3 = RB5 (RX), CCP4 = RD1 (TX), DS41364D 40-pin
     * PDIP pin diagram (PIC16F1937). Neither is APFCON-remapped by
     * pic16f193x_ccp.c, so the POR default location applies to both. */
    else if (tx_port == GPIOD && tx_pin == GPIO_PIN_1 &&
             rx_port == GPIOB && rx_pin == GPIO_PIN_5 && g_chan_b == NULL) {
        rx_inst = SWUART_CCP_RX_B; tx_inst = SWUART_CCP_TX_B;
        rx_cb = on_rx_event_b; tx_cb = on_tx_event_b;
        use_slot_b = 1u;
    }
#endif
    else {
        return EPIC_INVALID;
    }

    h->tx_port = tx_port; h->tx_pin = tx_pin;
    h->rx_port = rx_port; h->rx_pin = rx_pin;
    h->tx_state = TX_IDLE; h->tx_deadline = 0u;
    h->tx_head = h->tx_tail = h->tx_count = 0u;
    h->rx_state = RX_IDLE; h->rx_deadline = 0u;
    h->rx_head = h->rx_tail = h->rx_count = 0u;
    h->error_count = 0u;

    g_cycles_per_bit = compute_cycles_per_bit(fosc_hz, baud);

    EPIC_GPIO_Init(tx_port, tx_pin, GPIO_MODE_OUTPUT);
    /* Idle = mark (high). The TX CCP module starts in CCP_MODE_OFF and
     * won't drive the pin until the first real bit's compare event
     * fires, so without this the pin sits at whatever the latch last
     * held, which can be low, i.e. a break condition on a real wire. */
    EPIC_GPIO_WritePin(tx_port, tx_pin, GPIO_PIN_SET);
    EPIC_GPIO_Init(rx_port, rx_pin, GPIO_MODE_INPUT);

    s_timer1 = (TIMER1_HandleTypeDef)TIMER1_HANDLE_DEFAULT;
    EPIC_TIMER1_Init(&s_timer1);
    EPIC_TIMER1_Start(&s_timer1);

    CCP_HandleTypeDef ccp_rx = { .Instance = rx_inst, .Mode = CCP_MODE_CAPTURE_FALLING,
                                 .CompareValue = 0u, .EventCallback = rx_cb };
    EPIC_CCP_Init(&ccp_rx);
    CCP_HandleTypeDef ccp_tx = { .Instance = tx_inst, .Mode = CCP_MODE_OFF,
                                 .CompareValue = 0u, .EventCallback = tx_cb };
    EPIC_CCP_Init(&ccp_tx);

    EPIC_IRQ_Restore(1);
#if EPIC_SWUART_MAX_CHANNELS >= 2
    if (use_slot_b) { g_chan_b = h; } else { g_chan_a = h; }
#else
    g_chan_a = h;
#endif
    return EPIC_OK;
}

EPIC_StatusTypeDef EPIC_SWUART_DeInit(EPIC_SWUART_HandleTypeDef *h)
{
    if (!h) return EPIC_INVALID;
    if (g_chan_a == h) {
        EPIC_CCP_DeInit(SWUART_CCP_RX);
        EPIC_CCP_DeInit(SWUART_CCP_TX);
        g_chan_a = NULL;
    }
#if EPIC_SWUART_MAX_CHANNELS >= 2
    else if (g_chan_b == h) {
        EPIC_CCP_DeInit(SWUART_CCP_RX_B);
        EPIC_CCP_DeInit(SWUART_CCP_TX_B);
        g_chan_b = NULL;
    }
#endif
    else {
        return EPIC_INVALID;
    }
    /* EPIC_CCP_DeInit only zeroes CCPxCON; it doesn't touch the pin's
     * latch. Force the TX line back to mark (idle) so it doesn't linger
     * at whatever level the last bit left it, which can be low. */
    EPIC_GPIO_WritePin(h->tx_port, h->tx_pin, GPIO_PIN_SET);
    /* Timer1 is a shared resource: on PIC16F193X both channels run off
     * the same instance, so it may only be torn down once neither slot
     * is using it any more (both branches above already cleared their
     * own slot before reaching here). Single-channel families never
     * compile g_chan_b at all, so the condition there collapses to
     * exactly what Tasks 4-5 already had: g_chan_a == NULL. */
#if EPIC_SWUART_MAX_CHANNELS >= 2
    if (g_chan_a == NULL && g_chan_b == NULL) {
        EPIC_TIMER1_DeInit();
    }
#else
    if (g_chan_a == NULL) {
        EPIC_TIMER1_DeInit();
    }
#endif
    return EPIC_OK;
}

size_t EPIC_SWUART_Write(EPIC_SWUART_HandleTypeDef *h, const uint8_t *data, size_t len)
{
    if (!h) return 0u;
    size_t written = 0u;
    while (written < len && h->tx_count < EPIC_SWUART_RING_SZ) {
        /* No GIE manipulation (class-G conversion): the ring fields are
         * single bytes (atomic RMWs) and the write-before-increment
         * ordering holds (see EPIC_SWUART_Read's discipline note), and
         * the TX-arm pop below is mutually exclusive with the ISR's
         * per-bit pop: the arm only runs at TX_IDLE, the ISR only pops
         * during TX_DATA, and the state transition happens before the
         * ISR's first event. */
        h->tx_ring[h->tx_head] = data[written];
        h->tx_head = (uint8_t)((h->tx_head + 1u) & (EPIC_SWUART_RING_SZ - 1u));
        h->tx_count++;
        written++;
    }
    if (written > 0u && h->tx_state == TX_IDLE) {
        /* Dispatch to the handle's own slot, the same way DeInit already
         * does (g_chan_a vs g_chan_b), instead of hardcoding channel A's
         * CCP instance: a Write() on channel B must arm CCP4, not CCP2.
         * Defaults to slot A's instance so single-channel families (no
         * g_chan_b symbol at all) compile this down to a plain
         * assignment, zero branches, matching how the #if guard already
         * keeps DeInit's dual-channel branch out of their build. */
        CCP_InstanceTypeDef tx_inst = SWUART_CCP_TX;
#if EPIC_SWUART_MAX_CHANNELS >= 2
        if (g_chan_b == h) {
            tx_inst = SWUART_CCP_TX_B;
        }
#endif
        h->tx_shift = h->tx_ring[h->tx_tail];
        h->tx_tail = (uint8_t)((h->tx_tail + 1u) & (EPIC_SWUART_RING_SZ - 1u));
        h->tx_count--;
        h->tx_bit_index = 0u;
        h->tx_state = TX_DATA;
        h->tx_deadline = (uint16_t)(EPIC_TIMER1_ReadCounter() + SWUART_LEAD_CYCLES);
        EPIC_CCP_SetCompare(tx_inst, h->tx_deadline);
        EPIC_CCP_SetMode(tx_inst, CCP_MODE_COMPARE_CLEAR);
    }
    return written;
}

int EPIC_SWUART_Read(EPIC_SWUART_HandleTypeDef *h, uint8_t *buf, size_t maxlen)
{
    if (!h) return 0;
    size_t n = 0u;
    while (n < maxlen && h->rx_count > 0u) {
        /* Single-byte ring discipline, no GIE manipulation (class-G
         * conversion): rx_head/tail/count are single bytes (atomic
         * RMWs), the ISR push writes the slot before incrementing
         * count, and this read consumes a slot only after the count
         * check, so a torn or half-written slot cannot be observed. */
        buf[n] = h->rx_ring[h->rx_tail];
        h->rx_tail = (uint8_t)((h->rx_tail + 1u) & (EPIC_SWUART_RING_SZ - 1u));
        h->rx_count--;
        n++;
    }
    return (int)n;
}

uint16_t EPIC_SWUART_GetErrorCount(const EPIC_SWUART_HandleTypeDef *h)
{
    if (!h) return 0u;
    /* 16-bit read, no GIE manipulation (class-G conversion, the
     * epic_tick_get read-twice-retry pattern): a single Disable-guarded
     * read can tear (the ISR increments error_count as a multi-byte
     * RMW) and the Disable exposes the Finding 10.1 hazard. Retry until
     * two consecutive reads agree. */
    uint16_t count;
    do {
        count = h->error_count;
    } while (count != h->error_count);
    return count;
}
