# epic-swuart API reference

## `EPIC_SWUART_Init`

```c
EPIC_StatusTypeDef EPIC_SWUART_Init(EPIC_SWUART_HandleTypeDef *h,
                                     GPIO_TypeDef tx_port, uint16_t tx_pin,
                                     GPIO_TypeDef rx_port, uint16_t rx_pin,
                                     uint32_t fosc_hz, uint32_t baud);
```

**`tx_port`/`tx_pin`/`rx_port`/`rx_pin` are not freely choosable.** RX
uses CCP Capture mode and TX uses CCP Compare mode (see
`docs/ARCHITECTURE.md`); both are wired to specific physical pins per
CCP instance, so each channel "slot" has a fixed pin pair the caller
must match exactly:

| Family | Channel slot | TX pin (must match) | RX pin (must match) |
|---|---|---|---|
| PIC16F87XA | A (only slot) | `GPIOC`/`GPIO_PIN_1` (CCP2) | `GPIOC`/`GPIO_PIN_2` (CCP1) |
| PIC18Fxx5x | A (only slot) | `GPIOC`/`GPIO_PIN_1` (CCP2) | `GPIOC`/`GPIO_PIN_2` (CCP1) |
| PIC16F193X | A | `GPIOC`/`GPIO_PIN_1` (CCP2) | `GPIOC`/`GPIO_PIN_2` (CCP1) |
| PIC16F193X | B | `GPIOD`/`GPIO_PIN_1` (CCP4) | `GPIOB`/`GPIO_PIN_5` (CCP3) |

Passing any other pin combination returns `EPIC_INVALID`, the same
status returned for a `NULL` handle or a slot that's already occupied
by a previously-`Init`'d handle. `EPIC_SWUART_MAX_CHANNELS` (the
number of slots) is a real per-family hardware ceiling, not a
user-configurable value: `1` on PIC16F87XA/PIC18Fxx5x (2 CCP modules
total, both spent on slot A), `2` on PIC16F193X (5 CCP modules: slots
A and B use 4, 1 spare). It is resolved by family detection inside
`epic_swuart.h`.

Configures `tx_pin` as output, idling high (mark) immediately, and
`rx_pin` as input. The first `Init` call for any slot also starts
Timer1 (shared across every slot) and initializes that slot's two CCP
modules, RX in `CCP_MODE_CAPTURE_FALLING` and TX in `CCP_MODE_OFF`
(no deadline armed yet; see `docs/ARCHITECTURE.md`'s
`SWUART_LEAD_CYCLES` section for why TX starts off instead of armed).

Only `baud = 9600` is validated. The parameter exists because the
reload-value math is baud-parametric, not because other rates are
promised to work.

**Precondition:** the RX pin must idle high (pulled up, or connected to a
live transmitter) whenever the channel is not actively receiving; a floating
or held-low RX pin will be misread as a continuous stream of start bits.

**Real-hardware verification status:** TX has been verified byte-exact
correct on real compiled firmware (a `PORTC` hand-trace against MPLAB
SIM); RX has not been verified correct on real hardware. See
`docs/ARCHITECTURE.md`'s "Real-hardware verification" section for the
full, disclosed limitation before relying on this module's RX path in
a real deployment.

## `EPIC_SWUART_DeInit`

```c
EPIC_StatusTypeDef EPIC_SWUART_DeInit(EPIC_SWUART_HandleTypeDef *h);
```

Tears down `h`'s slot: both of that slot's CCP modules are
de-initialized (`EPIC_CCP_DeInit`), and the TX pin is forced back to
idle/mark (`GPIO_PIN_SET`) regardless of what the state machine was
doing mid-frame, so a channel removed mid-transmission doesn't leave
the wire held low (a break condition; see `docs/ARCHITECTURE.md`'s "TX
idle-mark guarantee"). Timer1 is a resource shared by every slot (on
PIC16F193X, both channel A and channel B run off the same instance):
it is only fully released (`EPIC_TIMER1_DeInit`) once no slot is left
active, not on every individual `DeInit` call. A later `Init` restarts
it from a clean state. Returns `EPIC_INVALID` if `h` is `NULL` or does
not match any currently-active slot.

## `EPIC_SWUART_Write`

```c
size_t EPIC_SWUART_Write(EPIC_SWUART_HandleTypeDef *h, const uint8_t *data, size_t len);
```

Enqueues up to `len` bytes into `h`'s TX ring, never blocks. Returns the
number actually queued: fewer than `len` when the ring fills up first
(a short write, not an error). If the TX state machine was idle, this
also arms the channel's TX CCP module for the new byte's start bit, at
an absolute Timer1 deadline `SWUART_LEAD_CYCLES` cycles in the future
(see `docs/ARCHITECTURE.md`); the actual bit-by-bit transitions from
then on happen in hardware, driven by the TX CCP compare event
handler, not by this function.

## `EPIC_SWUART_Read`

```c
int EPIC_SWUART_Read(EPIC_SWUART_HandleTypeDef *h, uint8_t *buf, size_t maxlen);
```

Drains up to `maxlen` received bytes into `buf`, never blocks. Returns
the number actually read (0 if the RX ring is empty).

## `EPIC_SWUART_GetErrorCount`

```c
uint16_t EPIC_SWUART_GetErrorCount(const EPIC_SWUART_HandleTypeDef *h);
```

Running total of dropped bytes since `Init`: a bad stop bit (framing
error) or an RX ring that was full when a byte finished receiving. Not
split by cause; both increment the same counter. The read is
IRQ-protected (`error_count` is a 16-bit field the CCP event handler
writes; an unprotected two-byte read could observe a torn value
mid-increment).
