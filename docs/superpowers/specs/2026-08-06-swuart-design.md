# Bit-banged software UART (epic-swuart), design

Status: **implemented 2026-08-06, timing engine superseded 2026-08-07**.
A final review measured the real compiled ISR on hardware at roughly 3x
over its cycle budget (continuous 3x-oversampling divides the CPU's
response budget by 3 for no benefit the design actually needed). The
API, framing, error handling, and module boundaries below are still
accurate and unchanged; the timing engine (Decisions: "Timing mechanism"
row, and the Architecture section below) is replaced by
`docs/superpowers/specs/2026-08-07-swuart-v2-design.md`'s edge-triggered,
one-timer-event-per-bit design. Read that document for the current
timing architecture; this one is kept for the parts still in force and
as the historical record of why the original approach was chosen and
where it fell short in practice.

## Problem

The three families each have exactly one hardware EUSART. An application
that needs a second serial channel (a sensor with its own UART, a debug
port that can't share the hardware EUSART, a bridge between two UART
devices) has nowhere to get one without dedicating GPIO pins and driving
the protocol in software. This module adds that: a bit-banged UART,
usable on any GPIO pin, with two independent channels able to run at the
same time.

## Decisions

| Question | Decision |
|---|---|
| Scope | Full duplex (RX + TX), not TX-only |
| Concurrency | Two channels active at once, alongside the hardware EUSART if also in use |
| Baud rate | 9600 only, validated and guaranteed. `baud` is still a runtime parameter (the math is baud-parametric), but nothing beyond 9600 is tested or promised |
| Framing | 8 data bits, no parity, 1 stop bit (8N1), not configurable in v1 |
| Timing mechanism | One shared hardware timer (Timer1) ticking at a fixed oversample rate, servicing every active channel's RX/TX state machine each tick. No pin-change/edge interrupt involved |
| Family branching | None. Built entirely on the fixed-contract HAL (`EPIC_GPIO_*`, `EPIC_TIMER1_*`, `EPIC_IRQ_*`), which is already identical across all three families |
| API shape | Handle-based (not a singleton like `epic-serial`), non-blocking, ring-buffered read/write |

## Why a shared timer tick, not edge-interrupt wakeup

Two alternatives were considered and rejected:

- **Edge-interrupt (pin-change) start-bit detection + reprogrammed
  one-shot timer per bit.** Lower idle CPU cost, but inherits a real pin
  restriction: PIC16F87XA/PIC18Fxx5x's port-change interrupt only
  covers RB4:RB7 (PIC16F193X's IOC covers any PORTB pin, per-pin edge
  select, so this constraint is family-specific, not universal). It
  also means juggling one timer's one-shot reloads across two
  independent, unsynchronized channels plus TX's own timing, which is
  fiddlier to get right and to test than a fixed tick.
- **Busy-wait bit-banging**, no timer or interrupts at all. Simplest
  code, but blocking: the CPU is stuck for the duration of any transfer,
  which rules out two channels being active at once (the actual
  requirement) and stalls the rest of the application meanwhile.

The shared fixed-rate tick is the only one of the three that generalises
cleanly to N concurrent channels, works on any GPIO pin regardless of
family, and matches `epic-serial`'s existing non-blocking philosophy.

## Architecture

New module: `epic-swuart`, same layout as every other module
(`README.md`, `docs/ARCHITECTURE.md`, `docs/API.md`, host CMake build,
real-target manifest entry).

Unlike `epic-serial` (one `#if` branch per family, because it drives the
real USART's registers directly), `epic-swuart` only calls fixed-contract
HAL functions: `EPIC_GPIO_Init`/`ReadPin`/`WritePin`,
`EPIC_TIMER1_Init`/`Start` with an `OverflowCallback`, `EPIC_IRQ_*`.
`TIMER1_HandleTypeDef`'s shape and `EPIC_GPIO_ReadPin`'s signature are
already identical across all three families. So this module is **one
`.c` file, zero `#ifdef`s, zero per-family bodies**, a first among this
repo's higher-level modules.

The module owns Timer1 exclusively for as long as any channel is active,
documented as a reserved resource the same way `epic_tick`'s README
documents owning Timer2. The reload value is computed from `FOSC_HZ` at
init time (the same "pick the configuration closest to the target
period" pattern `epic_tick.c` already uses for Timer2), not hardcoded
per family.

## API

```c
typedef struct { /* plain struct, like every handle in this HAL */ } EPIC_SWUART_HandleTypeDef;

EPIC_StatusTypeDef EPIC_SWUART_Init(EPIC_SWUART_HandleTypeDef *h,
                                     GPIO_TypeDef tx_port, uint16_t tx_pin,
                                     GPIO_TypeDef rx_port, uint16_t rx_pin,
                                     uint32_t fosc_hz, uint32_t baud);
EPIC_StatusTypeDef EPIC_SWUART_DeInit(EPIC_SWUART_HandleTypeDef *h);
size_t             EPIC_SWUART_Write(EPIC_SWUART_HandleTypeDef *h, const uint8_t *data, size_t len);
int                EPIC_SWUART_Read(EPIC_SWUART_HandleTypeDef *h, uint8_t *buf, size_t maxlen);
uint16_t           EPIC_SWUART_GetErrorCount(const EPIC_SWUART_HandleTypeDef *h);
```

No dynamic allocation: handles register into a small static array at
init time, sized by `EPIC_SWUART_MAX_CHANNELS` (default 2, compile-time
configurable). Each handle carries its own TX/RX ring buffers, sized by
`EPIC_SWUART_RING_SZ`: a compile-time `-D` override, same convention as
`epic-serial`'s `EPIC_SERIAL_RING_SZ` (not necessarily the same numeric
default; each handle needs two rings, and there can be two handles, so
the exact default is an implementation-plan detail, not a design one).

`EPIC_SWUART_Write` returns the number of bytes actually queued (a short
write when the TX ring is nearly full), the same shape every other
ring-buffered write in this repo uses.

## Data flow

1. `EPIC_SWUART_Write` copies bytes into the handle's TX ring; if TX is
   idle it starts immediately.
2. The first `Init` call lazily starts Timer1 at the shared oversample
   tick (about 4x baud). Later `Init` calls just add the handle to the
   static registry, the timer is already running at the right rate.
3. Every tick, the one shared ISR walks each active handle: steps its TX
   bit-clock (drives the pin, pulls the next queued byte when the
   current one finishes) and its RX sampler (detects a start bit by sampling
   the pin for a low level, then confirms it with a resample at half-bit
   time to reject noise; samples the data bits at bit-center offsets; and on
   a complete byte either pushes it to the RX ring or bumps the error counter
   on a bad stop bit). The RX line must idle high whenever the channel is not
   actively receiving; a floating or held-low RX pin will be misread as a
   continuous stream of start bits.
4. `EPIC_SWUART_Read` drains the RX ring, non-blocking.

## Error handling

One error counter per handle, not split by cause (splitting later is a
small, backward-compatible change if it turns out to matter):

- Bad stop bit (framing error): byte dropped, counter incremented, RX
  resyncs to idle and resumes sampling the RX pin for the next low-level
  start-bit indication.
- RX ring full: new byte dropped, counter incremented.
- `EPIC_SWUART_GetErrorCount(h)` exposes the running total.
- No runtime detection of a Timer1 resource conflict with application
  code (not generically possible); documented as reserved, same as
  `epic_tick` and Timer2.

## Testing

Follows this repo's existing per-module pattern rather than inventing a
new one:

- **Host sim**: no real timer or GPIO hardware exists on the host, so
  the test manually pumps the shared tick handler a deterministic
  number of times, mirroring `epic-serial`'s own
  `epic_dispatch_all_irqs` pump pattern, instead of relying on
  wall-clock interrupts. TX is tested by pumping ticks and inspecting
  the simulated pin's toggle sequence against the expected bit pattern.
  RX is tested by synthesising an inbound byte (setting the simulated
  RX pin level at the correct tick offsets) and checking the decoded
  byte lands in the read ring. Independent per-direction tests, not a
  physical loopback.
- **Real target**: `tests/example_swuart.c` (same shape as
  `example_blink.c`/`example_timer1.c`) for a hand-wired loopback
  (jumper TX to RX, or two channels crossed) as the manual verification
  path.

## Oversample-factor cycle budget, probed and verified

Per `docs/superpowers/plans/probe-swuart-isr-budget.md`: worst-case ISR
on PIC16F87XA at 20 MHz is 122 cycles. Budget at N=4 is 130 cycles
(6.2% headroom); budget at N=3 is 174 cycles (29.9% headroom). The
implementation adopts N=3 as the production default for its substantially
larger safety margin. N=4 is technically feasible with real headroom, but
not enough margin to add much more per-tick work without re-measuring;
N=3's ~30% margin provides the safety buffer needed for robust operation.

## What this design deliberately does not do

- Configurable parity or data/stop-bit counts. 8N1 only.
- Baud rates other than 9600. The parameter exists because the math is
  baud-parametric, but nothing else is validated.
- More than `EPIC_SWUART_MAX_CHANNELS` (default 2) concurrent channels.
- Runtime detection of a Timer1 conflict with other application code.
- Hardware flow control (RTS/CTS).
