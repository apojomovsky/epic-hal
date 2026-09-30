# epic-swuart, bit-banged full-duplex UART

Software UART on CCP-wired pins: a second serial channel (or, on
PIC16F193X only, a third and fourth) when the one hardware EUSART is
already spoken for. Timing is driven entirely by CCP (Capture/Compare)
hardware, not software polling: the start-bit edge is hardware-
timestamped and every TX bit transition happens at a hardware-armed
instant, both immune to ISR service latency.

- **CCP-wired pins, fixed per family, not any GPIO pin**: RX/TX are
  wired to a specific CCP module pair per channel slot; see
  `docs/ARCHITECTURE.md`'s allocation table. `EPIC_SWUART_Init` rejects
  any other pin combination with `EPIC_INVALID`.
- **Channel capacity is a real per-family hardware ceiling, not a
  compile-time option**: `EPIC_SWUART_MAX_CHANNELS` is 1 on
  PIC16F87XA/PIC18Fxx5x (2 CCP modules total, all spent on one channel)
  and 2 on PIC16F193X (5 CCP modules, enough for two channels with one
  spare). Resolved by family detection in `epic_swuart.h`, not
  user-overridable.
- **9600 baud, 8N1**, non-blocking ring-buffered read/write, same shape
  as `epic-serial`'s.
- **Owns Timer1 and its channels' CCP instances** for as long as any
  channel is active. Do not also drive Timer1 or those CCP instances
  directly from application code while a channel is initialised.

## Documentation

- [Architecture](docs/ARCHITECTURE.md): the CCP capture/compare design,
  the free-running-Timer1 absolute-deadline model, the per-family pin
  table, and the documented hazards.
- [API reference](docs/API.md): per-function semantics + usage.

## Quick start

### Host simulator (the tests)

```sh
cmake -B build && cmake --build build
ctest --test-dir build --output-on-failure
cmake -B build18 -DEPIC_FAMILY=PIC18 && cmake --build build18 && ctest --test-dir build18
cmake -B build193x -DEPIC_FAMILY=PIC16F193X && cmake --build build193x && ctest --test-dir build193x
```

Host-sim tests cannot exercise real CCP capture/compare hardware (no
host simulator models it); they drive the TX/RX state machine directly
through test-only accessor functions compiled in only for host-sim
builds. See `docs/ARCHITECTURE.md`'s "Host testing" section.

### Real target (XC8), echo demo

The target-only demo (`examples/example_swuart.c`) opens channel A
(TX = RC1, RX = RC2 at 9600 baud 8N1), sends a banner, and echoes
every received byte back. Connect a serial terminal to RC1/RC2 and
type; RB1 toggles per echoed byte.

```sh
export PATH=$PATH:/opt/microchip/xc8/v3.10/bin
python3 scripts/epic_build.py build --module epic-swuart --mcu 16F877A --run
```

**Real-hardware verification status:** the module's `mdb` gate
(`scripts/ci-target-sim.sh`) proves real CCP-compare-driven TX firmware
runs to completion under MPLAB SIM without hanging, and a separate
hand-trace confirmed the compiled `.hex`'s TX bit sequence is
byte-exact hardware-correct. It does **not** prove real-hardware RX
correctness: two different loopback approaches were attempted and both
hit real, unresolved obstacles. See `docs/ARCHITECTURE.md`'s
"Real-hardware verification" section for the full, honest disclosure.

## Use it

```c
#include "epic_swuart.h"

EPIC_SWUART_HandleTypeDef h;
/* GPIOC/PIN_1 (TX) and GPIOC/PIN_2 (RX) are channel A's fixed CCP
 * pins on every family; see docs/ARCHITECTURE.md for the full table
 * and for PIC16F193X's second-channel pins. */
EPIC_SWUART_Init(&h, GPIOC, GPIO_PIN_1, GPIOC, GPIO_PIN_2, FOSC_HZ, 9600u);
EPIC_SWUART_Write(&h, (const uint8_t *)"hi\r\n", 4);
uint8_t buf[8];
int n = EPIC_SWUART_Read(&h, buf, sizeof(buf));  /* non-blocking RX */
```

## License

MIT, see the [repo LICENSE](../LICENSE).
