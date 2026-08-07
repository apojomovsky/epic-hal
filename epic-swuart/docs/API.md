# epic-swuart API reference

## `EPIC_SWUART_Init`

```c
EPIC_StatusTypeDef EPIC_SWUART_Init(EPIC_SWUART_HandleTypeDef *h,
                                     GPIO_TypeDef tx_port, uint16_t tx_pin,
                                     GPIO_TypeDef rx_port, uint16_t rx_pin,
                                     uint32_t fosc_hz, uint32_t baud);
```

Configures `tx_pin` as output (idling high, mark), `rx_pin` as input,
and registers `h` in the shared channel registry. The first call also
starts Timer1 at the shared oversample tick; later calls just add to the
registry. Returns `EPIC_INVALID` if `h` is `NULL` or
`EPIC_SWUART_MAX_CHANNELS` channels are already registered.

Only `baud = 9600` is validated. The parameter exists because the
reload-value math is baud-parametric, not because other rates are
promised to work.

**Precondition:** the RX pin must idle high (pulled up, or connected to a
live transmitter) whenever the channel is not actively receiving; a floating
or held-low RX pin will be misread as a continuous stream of start bits.

## `EPIC_SWUART_DeInit`

```c
EPIC_StatusTypeDef EPIC_SWUART_DeInit(EPIC_SWUART_HandleTypeDef *h);
```

Removes `h` from the registry (the shift-and-decrement is IRQ-protected,
matching `Write`/`Read`'s ring-mutation protection: a tick landing
mid-removal would otherwise see a stale count and service the
surviving channel twice). Leaves `h`'s TX pin at idle/mark
(`GPIO_PIN_SET`) regardless of what the state machine was doing
mid-frame, so a channel removed mid-transmission doesn't leave the
wire held low (a break condition). Fully releases Timer1
(`EPIC_TIMER1_DeInit`, not just `EPIC_TIMER1_Stop`) once no channel is
left active, so the interrupt source goes with it, not just the
counter; a later `Init` restarts it from a clean state.

## `EPIC_SWUART_Write`

```c
size_t EPIC_SWUART_Write(EPIC_SWUART_HandleTypeDef *h, const uint8_t *data, size_t len);
```

Enqueues up to `len` bytes into `h`'s TX ring, never blocks. Returns the
number actually queued: fewer than `len` when the ring fills up first
(a short write, not an error).

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
IRQ-protected (`error_count` is a 16-bit field the ISR writes; an
unprotected two-byte read could observe a torn value mid-increment).
