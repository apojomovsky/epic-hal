# epic-usb, USB CDC-ACM virtual serial port for PIC18F2455/2550/4455/4550

A `epic_serial_*`-shaped non-blocking USB CDC-ACM device, wrapping the
vendored M-Stack USB device stack (`third_party/m-stack`).

- **Mirrors epic-serial's API**: same `write`/`read`/`available`/`flush`
  contract, same ring-buffer discipline (TX demand-driven, RX always-on),
  plus `connected()` — true once the host opens the CDC port (DTR asserted),
  not merely once enumeration finishes.
- **PIC18Fxx5x-only**: the PIC18F2455/2550/4455/4550 SIE peripheral has no
  equivalent on PIC16F87XA parts (no USB peripheral at all), so there is no
  PIC16 backend and never will be. Not a scope choice — a hardware constraint.
- **Polling, not interrupts**: `epic_usb_service()` pumps M-Stack's
  enumeration and endpoint state machine; call it every main-loop iteration or
  from a short-period `epic-taskmgr` task. M-Stack's interrupt-driven mode is
  deliberately not wired up.
- **Host stub tests the contract, not the silicon**: the host build links a
  separate `epic_usb_host_stub.c` (not a compile-time swap of the same
  source) that proves the ring-buffer and connection-state logic — it does
  **not** simulate USB enumeration, and no host test substitutes for real
  silicon bring-up.

## Documentation

- [Architecture](docs/ARCHITECTURE.md), why this module breaks the
  family-agnostic pattern, the host-build boundary, ring-buffer design,
  polling vs interrupts.
- [API reference](docs/API.md), per-function semantics + usage.
- [Implementation plan](../docs/epic-usb-plan.md), M-Stack vendoring
  rationale, the `usb_hal.h` port, Phase 2 XC8 findings, open risks.

## Quick start

### Host simulator (the test)

```sh
cmake -B build -S epic-usb && cmake --build build
ctest --test-dir build --output-on-failure
```

### Real target (XC8)

Real-silicon bring-up is Phase 3 — the `mcu/pic18fxx5x-usb-mplabx/Makefile`
does not exist yet. Phase 2 confirmed that `epic_usb.c` + M-Stack's core +
CDC class driver compile and link clean under `xc8-cc` against real
PIC18F4550 headers (8500 bytes flash / 25.9%, 454 bytes RAM / 22.2% of
2 KiB). No PIC16 `mcu/` directory will ever be added — that family has no
USB peripheral.

## Use it

```c
#include "epic_usb.h"
epic_usb_init();                             /* once, starts enumeration */

for (;;) {
    epic_usb_service();                      /* pump the stack frequently */
    if (epic_usb_connected()) {              /* host has the port open */
        uint8_t buf[64];
        size_t n = epic_usb_read(buf, sizeof(buf));  /* non-blocking */
        if (n) epic_usb_write(buf, n);       /* echo back */
    }
}
```

## Third-party license

`third_party/m-stack/` vendors [M-Stack](https://github.com/signal11/m-stack)
under the Apache-2.0 arm of its dual LGPLv3 / Apache-2.0 license. M-Stack's
own `LICENSE-*.txt` files are left untouched inside that directory. Remove it
from your fork if you do not want to redistribute third-party code.

The default VID/PID (0xA0A0 / 0x0004) is M-Stack's bundled placeholder —
fine for bench bring-up, **not for redistribution**. Replace with a real
pid.codes allocation before shipping a device.

## License

MIT, see the [repo LICENSE](../LICENSE).
