# lib/usb, USB CDC-ACM virtual serial port for PIC18F2455/2550/4455/4550

A `epic_serial_*`-shaped non-blocking USB CDC-ACM device, wrapping the
vendored M-Stack USB device stack (`third_party/m-stack`).

- **Mirrors lib/serial's API**: same `write`/`read`/`available`/`flush`
  contract, same ring-buffer discipline (TX demand-driven, RX always-on),
  plus `connected()` — true once the host opens the CDC port (DTR asserted),
  not merely once enumeration finishes.
- **PIC18Fxx5x-only**: the PIC18F2455/2550/4455/4550 SIE peripheral has no
  equivalent on PIC16F87XA parts (no USB peripheral at all), so there is no
  PIC16 backend and never will be. Not a scope choice — a hardware constraint.
- **Polling, not interrupts**: `epic_usb_service()` pumps M-Stack's
  enumeration and endpoint state machine; call it every main-loop iteration or
  from a short-period `lib/taskmgr` task. M-Stack's interrupt-driven mode is
  deliberately not wired up.
- **Host stub tests the contract, not the silicon**: the host build links a
  separate `epic_usb_host_stub.c` (not a compile-time swap of the same
  source) that proves the ring-buffer and connection-state logic — it does
  **not** simulate USB enumeration, and no host test substitutes for real
  silicon bring-up.

## Bring-up phases

| Phase | What it covers | State |
|---|---|---|
| 1 | Vendor M-Stack, confirm its actual API surface, port `usb_hal.h` to the PIC18F2455/2550/4455/4550 | Done |
| 2 | The `epic_usb_*` wrapper, the host stub, the unit tests, a real XC8 build against PIC18F4550 headers | Done |
| 3 | Silicon bring-up: the bring-up project, the simulator gate, and the hardware enumeration + CDC echo gate | Project and simulator gate done; the hardware half is unproven (no board on the machine that built it) |

Phase 3's project lives in
[`mcu/pic18fxx5x-usb-mplabx/`](mcu/pic18fxx5x-usb-mplabx/README.md), and
its gate is `make usb-gate MODULE=lib/usb MCU=18F4550`.

## Simulator coverage

The `usb-cdc` example has an MPLAB SIM gate (`tests/sim_usb.c`, the
manifest's `sim` entry, so the module page reports
`Simulator gate: PIC18Fxx5x`). It runs the real firmware and reads back
the SIE state: the SIE is enabled, full speed is selected, the internal
pull-up is on, EP0 is a control endpoint with handshaking, EP1/EP2 carry
data, and every buffer descriptor at 400h points into USB RAM 500h-7FFh
(DS39632E Section 17.4.3).

MPLAB SIM models no USB SIE: no bus, no host, no token engine. The gate
therefore proves the SIE is programmed and the buffers are reachable, and
proves nothing about enumeration. The epic-cc simulator has no SIE model
either. Enumeration is proven by the hardware gate, which needs a board:
it finds the CDC-ACM port, opens it, asserts DTR and requires 64 bytes
to come back byte-exact.

The host build links `src/epic_usb_host_stub.c`, a test double that shares
no code with `src/epic_usb.c`. It proves the public API contract only,
including ring fill and drain, overflow drop, DTR-gated `connected()`,
flush draining while connected, and short non-blocking reads, driven
through the `epic_usb_test_support.h` hooks (`set_dtr`, `inject_rx`, and
the sent log). It does not prove USB enumeration, endpoint 0 control
transfers, descriptor contents, CDC class requests, or bus timing.

## Documentation

- [API reference](docs/API.md), per-function semantics + usage.
- [Bring-up project](mcu/pic18fxx5x-usb-mplabx/README.md), the board
  requirements, the gates and what each proves.

## Quick start

### Host simulator (the test)

```sh
cmake -B build -S lib/usb && cmake --build build
ctest --test-dir build --output-on-failure
```

### Real target (XC8)

```sh
make xc8-build MODULE=lib/usb MCU=18F4550
```

The hex lands at `build/18F4550-usb-cdc.hex`. The bring-up project's
MPLAB X project builds the same firmware from the IDE. No PIC16 `mcu/`
directory will ever be added: that family has no USB peripheral.

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

MIT, see the [repo LICENSE](../../LICENSE).
