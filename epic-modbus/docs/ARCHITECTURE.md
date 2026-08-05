# `epic-modbus` architecture

A Modbus RTU slave, silence-delimited framing on top of `epic-serial` (the
UART) and `epic-tick` (the T3.5 timebase), the design work is recorded in
[`docs/epic-modbus-plan.md`](../../docs/epic-modbus-plan.md); this file is
the as-built version.

## What it is

One slave instance per firmware image (file-scope statics, the same
single-instance model `epic-serial`/`epic-tick` use, not a caller-owned
handle). `epic_modbus_slave_poll()` is the entire integration surface:
called every main-loop iteration (or as a `epic-taskmgr` task), it drains
newly received UART bytes, detects the RTU T3.5 inter-frame silence, and
when a full frame has arrived, validates and dispatches it in the same
call.

## Why silence-delimited, not event-delimited (no `epic-fsm`)

RTU frames have no length field or delimiter byte, the *only* thing that
marks a frame boundary is a gap of at least T3.5 character-times of bus
silence. That is fundamentally a timestamp comparison ("has enough time
passed since the last received byte"), not a sequence of discrete
events. `epic-fsm`'s transition-table model (rows keyed on
`(state, event)`) doesn't fit an algorithm whose only "event" is the
absence of further bytes for a while, so `epic-modbus` doesn't use it,
exactly the same call `epic-debounce` made about its own poll loop.

## The poll algorithm

```
avail = epic_serial_available()
if avail > 0:
    drain up to (EPIC_MODBUS_MAX_ADU - frame_len) bytes into frame[]
    discard anything beyond that (keeps the RX ring from wedging on an
    oversized frame; the frame fails length/CRC validation regardless)
    last_rx_tick = epic_tick_get()

if frame_len > 0 and epic_tick_elapsed_since(last_rx_tick) >= t3_5_ms:
    process_frame()      # validate + dispatch + respond
    frame_len = 0         # next byte always starts a fresh frame
```

`process_frame()`: reject frames shorter than 4 bytes (addr+fc+crc16
minimum); verify CRC-16; check the address (exact match, or broadcast 0);
dispatch by function code to a per-FC handler that returns either a success
PDU, an exception PDU (`[fc|0x80][exception_code]`), or 0 ("malformed
length", dropped silently, this is a defensive bounds check, not a
spec-required response, a frame with a valid CRC but wrong length for its
own function code is vanishingly unlikely to occur naturally). A broadcast
request (address 0) is fully processed (writes applied) but never gets a
transmitted response, per spec.

## T3.5 timing, and why it's approximate above 19200 baud

`epic-tick` provides a 1 ms monotonic counter. The spec's T3.5 (3.5
character-times of silence) is:

- **Baud ≤ 19200**: `ceil(3.5 * 11 bits/char * 1000 ms/s / baud)`, rounded
  *up* so the computed timeout never falls short of the true silence
  requirement (a shorter timeout would risk splitting one frame into two).
  At 9600 baud this is 5 ms; at 19200, 3 ms.
- **Baud > 19200**: the spec fixes T3.5 = 1.75 ms directly (the
  character-time formula becomes unreliable at high baud); `epic-tick`'s
  1 ms granularity rounds that up to 2 ticks.

**Known limitation**: at baud rates above ~19200, `epic-tick`'s 1 ms
resolution is coarser than the true T3.5 (e.g. the true value is ~304 µs at
115200, but the slave waits ~2 ms). Framing is still correct, silence
detection just fires a bit later than a byte-precise implementation would,
adding a little latency, not a correctness bug, as long as
`epic_modbus_slave_poll()` is called more often than T3.5. This is
acceptable for the 9600/19200 links these parts realistically see; a
hardware-timer-based sub-ms T3.5 would be the fix if strict high-baud
conformance is ever needed. Strict T1.5 inter-character corruption
detection (aborting a frame if the gap *between* two bytes within it
exceeds T1.5) is likewise not implemented, a well-behaved master never
triggers the case it exists to catch.

## CRC-16

A bit-loop (poly 0xA001, init 0xFFFF), private to `src/epic_modbus.c`. Not
table-driven: a 256-entry lookup table would cost ~512 B of flash to save a
few dozen cycles per frame, and RTU's own byte rate (at most a few thousand
bytes/second) makes that irrelevant. Not added to `epic-math`: that module
is fixed-point arithmetic (multiply/divide/sqrt/etc.), CRC is a different
kind of primitive and nothing else in the repo needs it yet, adding it
there would be an unused, untested surface.

## RAM budget: PIC16F876A/877A and every PIC18 family member fit; 873A/874A don't

Verified by building the real XC8 target: `MCU=16F877A` (368 B RAM) links
with the default `EPIC_MODBUS_MAX_ADU=64` at 98.4% data space, snug but
fitting; every PIC18F2455/2550/4455/4550 target (2 KB RAM) has plenty of
headroom (27.2% used). The 192 B PIC16F873A/874A variants **fail to link**
at default settings, "could not find space" for the ring buffers. This is
**inherited from `epic-serial` itself**: its own target build already fails
on `MCU=16F873A` with zero Modbus code linked at all (`g_rx_buf` alone is
32 B), confirmed by building `epic-serial/mcu/pic16f87xa-serial-mplabx`
standalone. Shrinking `EPIC_MODBUS_MAX_ADU` and `EPIC_SERIAL_RING_SZ`
together helps but doesn't close the gap (tried down to 8 bytes each,
still short by a handful of bytes once the full HAL peripheral set these
Makefiles link is accounted for). Fixing that is a `epic-serial`-level
concern, out of scope here; the realistic targets for `epic-modbus` are
PIC16F876A/877A and the PIC18Fxx5x family.

## Register map: plain arrays, not an ops seam

`epic-bus` injects a callback "ops" table so its host tests can mock
hardware the host simulator has no model for (a real I2C/SPI slave
device). `epic-modbus`'s register arrays aren't standing in for hardware,
they *are* the data, identically on the host sim and real silicon. A
callback layer here would only add indirection with no testability
benefit, so `epic_modbus_slave_map_t` is plain pointers + counts, matching
this repo's rule against introducing abstraction the problem doesn't need.

## RS-485 direction control

Optional, and family-neutral in the public header: `epic_modbus.h` takes
plain `uint8_t port, pin` (mirroring `epic_bus_spi_init`'s `cs_port`/
`cs_pin` convention), not a HAL `GPIO_TypeDef`, so the public header needs
no HAL include at all. Internally, `src/epic_modbus.c` casts to
`GPIO_TypeDef`/`EPIC_BIT(pin)`, the same idiom `epic-bus` uses for its SPI
chip-select pin.

If never configured, responses go out through `epic_serial_write`
untouched (fine for point-to-point wiring or an auto-sensing transceiver).
If configured, `send_response()` asserts the pin, writes the response, then
calls `epic_serial_flush()`, which blocks until both the TX ring *and* the
UART shift register are empty, before dropping the pin. Releasing the
driver before the stop bit has physically left the pin is exactly the
failure mode RS-485 transceiver datasheets warn about (a truncated last
byte on the bus); `flush`'s "ring AND shift register" guarantee is what
makes this safe.

## No family branch needed

Unlike `epic-bus` (whose SSP register access differs enough between PIC16
and PIC18 to need an `#if`), `epic-modbus` needs none at all: its only
per-family surface is GPIO, and that's already neutral through
`epic_hal.h`'s `GPIO_TypeDef`/`EPIC_GPIO_*` contract (same names, same
signatures, different bodies selected at the include-path/link level).
`src/epic_modbus.c` is the same file, unchanged, for both families.

## A note on the PIC16 hardware call-stack warning

Building the PIC16F87XA target smoke reports "possible hardware stack
overflow detected, estimated stack depth: 15" (`example_modbus_target.c`).
This is **not new**: `epic-serial`'s own target build, with no Modbus code
linked at all, already reports an estimated depth of 13 against the
PIC16F87XA's 8-level hardware return-address stack, XC8's call-graph
estimator sums worst-case mainline + interrupt nesting pessimistically.
`epic-modbus` adds two more levels of ordinary call depth
(`poll → process_frame → handle_*`) on top of that pre-existing baseline;
fixing the underlying estimate is out of scope for this module.

## Host testing

`examples/example_modbus.c` mirrors `epic-serial/examples/example_serial.c`:
RX bytes are injected through `*_sim_drive_usart_rx`, `epic_tick_delay_ms`
(which pumps `epic_harness_tick()`) advances simulated time past T3.5, and
TX bytes are captured by pumping `epic_dispatch_all_irqs` while reading
`EPIC_REG8(PIC_REG_TXREG)`. Expected CRC-16 values are computed with a
second, independently written copy of the CRC-16/MODBUS algorithm
(self-checked against the standard catalogue "123456789" → `0x4B37` test
vector), so the test cannot share a CRC bug with `src/epic_modbus.c`.
Covers: a read function code (FC03), a write function code (FC06, checking
both the side effect and the echoed response), an illegal-address exception
path, and the broadcast no-reply path.
