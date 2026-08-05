# `epic-modbus`: Modbus RTU slave, implementation plan

Status: **implemented; superseded by `epic-modbus/docs/ARCHITECTURE.md`**,
which now carries the as-built design (including findings only known after
building: RAM budget per MCU, the PIC16 hardware-stack-depth note). This
document is kept only for the pre-implementation process history (library
survey, scope confirmation) that has no as-built equivalent; the "Design"
and "Known limitations" sections below have been trimmed to a pointer since
their content is now duplicated, and kept current, there.

## Why this module, and why from scratch

Nothing in this repo implements Modbus or CRC-16 today. External libraries
were surveyed first (nanoMODBUS, FreeMODBUS, liblightmodbus, PetitModbus,
`pic-modbus`, libmodbus, esp-modbus): none is vendored in. nanoMODBUS (MIT,
static/no-malloc, two-callback transport) and FreeMODBUS (BSD, real bare-metal
AVR port with a `port.c`/`portserial.c`/`porttimer.c` split) are the two worth
reading for *design shape*; GPL-licensed options (liblightmodbus,
PetitModbus, `pic-modbus`) are ruled out outright for a permissively-licensed
HAL. This module is written the way `epic-math` ported AN526/AN544: read the
prior art for the idea, write idiomatic code for this repo's own HAL
contract.

## Scope (confirmed with the user)

- **Role**: RTU **slave** only. No master/client role.
- **Transport**: **RTU** binary framing + CRC-16 only. No ASCII, no TCP.
- **Function codes**: 01 (Read Coils), 02 (Read Discrete Inputs), 03 (Read
  Holding Registers), 04 (Read Input Registers), 05 (Write Single Coil), 06
  (Write Single Register), 15/0x0F (Write Multiple Coils), 16/0x10 (Write
  Multiple Registers).

## What it builds on (solved)

- **`epic-serial`**: non-blocking ring-buffered UART
  (`epic_serial_init/write/read/available/tx_pending/flush`). RTU byte I/O
  and RS-485 DE/RE timing (via `flush`, which blocks until the ring *and*
  shift register are both empty) come straight from here.
- **`epic-tick`**: 1 ms monotonic timebase
  (`epic_tick_get/elapsed_since`) drives the T3.5 inter-frame silence
  detection.
- **GPIO HAL** (`EPIC_GPIO_WritePin`/`GPIO_TypeDef`, per-family): optional
  RS-485 driver-enable pin.
- **Module template**: `epic-serial`'s file layout, and `epic-debounce`'s
  `CMakeLists.txt`/Makefile pattern for linking a sibling `epic-*` module
  (`epic-debounce` links `epic-tick` the same way `epic-modbus` needs to
  link both `epic-serial` and `epic-tick`).
- **Host test pattern**: `epic-serial/examples/example_serial.c`'s RX
  injection (`pic16f87xa_sim_drive_usart_rx` / PIC18 equivalent) + TX
  draining (`epic_dispatch_all_irqs` + reading `EPIC_REG8(PIC_REG_TXREG)`),
  combined with `epic_tick_delay_ms` to advance simulated time past T3.5.
- **Considered and rejected**: `epic-fsm` for RTU frame assembly. RTU
  framing is silence-delimited, not event-delimited; a direct
  accumulate-then-check-elapsed poll loop is simpler than forcing it through
  transition-table guards/actions, the same call `epic-debounce` made about
  not using `epic-fsm` for its own poll loop.

## Design, and known limitations

**Superseded, see `epic-modbus/docs/ARCHITECTURE.md`** for the public API,
the RTU framing algorithm, CRC-16, RS-485 direction control, and the T3.5
timing-resolution/T1.5 limitations, all in as-built form (the arrays/API
shapes below matched the original plan closely, but the as-built doc also
covers what could only be known after building: measured RAM budget per
MCU, and the PIC16 hardware-stack-depth note).

Still true regardless of doc: master/client role, ASCII framing, and TCP
transport remain out of scope, per the confirmed scope above.

## Files

```
epic-modbus/
  README.md
  docs/ARCHITECTURE.md
  docs/API.md
  CMakeLists.txt
  include/epic_modbus.h
  src/epic_modbus.c
  examples/example_modbus.c         # host ctest
  examples/example_modbus_target.c  # real-target link/init smoke
  mcu/pic16f87xa-modbus-mplabx/Makefile
  mcu/pic18fxx5x-modbus-mplabx/Makefile
```

## Verification

- `cmake -B build && cmake --build build && ctest --test-dir build
  --output-on-failure`, then `cmake -B build18 -DEPIC_FAMILY=PIC18 && ctest
  --test-dir build18`, both green.
- Host test asserts exact wire bytes (including computed CRC) for a read FC,
  a write FC, an exception path, and the broadcast no-reply path.
- `make -C mcu/pic16f87xa-modbus-mplabx MCU=16F877A` and the PIC18
  equivalent link cleanly (smoke only, no live transaction, same caveat as
  `epic-bus`'s target example).
