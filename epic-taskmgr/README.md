# 8-bit PIC Task Manager

[![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg)](../LICENSE)
[![Runs on: host & silicon](https://img.shields.io/badge/runs%20on-host%20%26%20silicon-orange.svg)](#quick-start)

A cooperative (non-preemptive) task scheduler for 8-bit PIC
microcontrollers, built on a [family-agnostic HAL](../epic-common). It is not
an RTOS: there is no preemption and no per-task stack, which suits a part
with no software stack and only modest RAM (192 B on a PIC16F873A up to 2 KB
on a PIC18F4550). Tasks are plain functions that run to completion in
priority order on each tick, like interrupt handlers.

The same `epic_taskmgr.c`/`epic_taskmgr.h` builds against any 8-bit PIC HAL
family (PIC16F87XA, PIC18F2455, ...) via the neutral `epic_hal.h` contract;
select the family at build time with `-DEPIC_FAMILY=PIC16` (default) or
`-DEPIC_FAMILY=PIC18`. See [epic-common/README.md](../epic-common/README.md)
and [epic-common/MANUAL.md](../epic-common/MANUAL.md) for the shared
contract.

> 📖 **Documentation**: [API reference](docs/API.md)

## Features

- **Spawn tasks** at startup or at runtime, including from inside a running task.
- **Periodic tasks** (every N ticks) and **one-shot tasks** (run once, then free
  their slot, so re-spawning one-shots never exhausts the table).
- **Priority ordering** within each round (lower number runs first).
- **Start / stop / set-period** at runtime.
- **Build-agnostic**: one source builds for the host simulator and a real XC8
  target with no `#ifdef`, via the HAL's harness seam.
- **RAM-aware**: 6 task slots on the 192 B PIC16F873A/874A, 8 on the 368 B
  PIC16F876A/877A; banks cleanly into every part. Override with
  `-DTASK_MGR_MAX_TASKS=N`. Note: the real-target example build is
  manifest-excluded on the 192 B parts (the full-HAL build's 54-byte
  default table does not fit there), so CI covers 16F876A/16F877A
  only.
- **Race-free**: the tick (interrupt context) and the run loop (main context)
  share the task table through short critical sections; a tick that lands
  during a long task arms it for the next round instead of being lost.

## Quick start

### Host simulator

Requires only CMake and a C compiler. The HAL is pulled in via
`add_subdirectory(../pic16f87xa-hal)` and linked automatically.

```sh
cmake -B build -S .
cmake --build build

./build/example_multi_blink                 # default device (PIC16F877A)
./build/example_multi_blink_PIC16F873A      # 28-pin, 192 B
```

The task manager is **family-agnostic**: it includes only the neutral HAL
contract headers (`epic_hal.h`, `core/hal_irq.h`,
`peripherals/hal_timer0.h`, `core/hal_wdt_sleep.h`) plus the shared
`epic_harness.h`, so the same `epic_taskmgr.c`/`.h` builds against any
8-bit PIC family. To build against the PIC18F2455 family instead
(`pic18fxx5x-hal`), pass `-DEPIC_FAMILY=PIC18`:

```sh
cmake -B build18 -S . -DEPIC_FAMILY=PIC18
cmake --build build18

./build18/example_multi_blink               # default device (PIC18F4550)
```

This is the multi-family litmus test (per the shared-contract design
in [epic-common/README.md](../epic-common/README.md)):
pointing the task manager at `pic18fxx5x-hal` needs zero changes to
`epic_taskmgr.c`/`epic_taskmgr.h` (only the 3 family-neutral include
lines, which are the same for either family). The PIC18 run produces the
same output shape (four rates, one spawned blip, slot reuse); exact tick
counts differ because Timer0 timing differs between families.

The example streams a dispatch log as it runs (host only; the target has no
stdout and runs forever):

```
[t= 20] fast  #4
[t= 20] med   #2
[t= 20] slow  #1
[t= 40] super  spawned blip
[t= 41] blip  #1
done: fast=12 med=6 slow=3 blips=1 (ticks=61, tasks=4)
```

Four blinks at distinct rates on RB0-RB3, plus a priority-0 supervisor that
spawns a one-shot blip at runtime. `tasks=4` (not 5) shows the blip freed its
slot after running, which is what lets the supervisor spawn blips indefinitely
without exhausting the table.

### Real target

Built with MPLAB XC8 v3.x (`xc8-cc`). The Makefile selects the real-target
platform header and links the target harness and interrupt vector; the
host-only simulator backend is not compiled. There is one Makefile per
HAL family:

```sh
export PATH=$PATH:/opt/microchip/xc8/v3.10/bin

# PIC16F87XA family (16F873A/16F874A are manifest-excluded from the
# real-target build: the full-HAL example does not fit their 192 B
# RAM):
python3 scripts/epic_build.py build --module epic-taskmgr --mcu 16F877A --run
python3 scripts/epic_build.py build --module epic-taskmgr --mcu 16F876A --run

# PIC18F2455 family (needs the PIC18Fxxxx DFP installed; see
# pic18fxx5x-hal/mcu/pic18fxx5x-mplabx/README.md):
python3 scripts/epic_build.py build --module epic-taskmgr --mcu 18F4550 --run
```

This produces `build/16F877A-multi-blink.hex` (or the MCU you built for).
Program it with MPLAB X or any external programmer (PICkit, ICD, IPE).

### Wiring (real target)

- **LEDs**: an LED and current-limiting resistor on each of RB0, RB1, RB2, RB3
  to GND (active-high).
- **Clock**: a 20 MHz HS crystal on OSC1/OSC2 (Fosc/4 = 5 MHz). With the default
  Timer0 prescaler 1:256 and reload 61, the tick is ~9.98 ms.
- **Config word** (auto-generated): `FOSC=HS, WDTE=ON, PWRTE=ON, BOREN=ON,
  LVP=OFF`. The scheduler refreshes the WDT each loop.

Expected behavior: RB0 blinks fastest (~50 ms), RB1 (~100 ms), RB2 (~200 ms),
and RB3 blips once every ~400 ms (a freshly spawned one-shot task).

## Use it in your own firmware

```c
#include "epic_taskmgr.h"

static void my_task(void *arg) {
    /* ...a small amount of work, then return... */
}

int main(void) {
    epic_taskmgr_init();
    epic_taskmgr_spawn(my_task, NULL, /*period_ticks=*/10, /*priority=*/1);

    epic_taskmgr_attach_timer0(61, TIMER0_PRESCALER_1_256);  /* ~10 ms tick */
    EPIC_IRQ_Restore(1);   /* arm the Timer0 interrupt */

    epic_taskmgr_run();          /* never returns on target */
}
```

For a custom main loop, call `epic_taskmgr_run_once()` directly instead of
`epic_taskmgr_run()`. To drive the tick from another timer, call
`epic_taskmgr_tick()` from that timer's ISR instead of using
`epic_taskmgr_attach_timer0()`; the scheduler is agnostic to the tick source.
See the [API reference](docs/API.md).

## License

MIT; see the top-level [LICENSE](../LICENSE).
