# Combo gates (tests/)

The `epic-combo-*` firmwares are ISR-interleave gates: two or more
drivers running under a live interrupt vector in one image, asserting
exact behavior (counts, bytes, dispatch ratios) over UART through the
mdb harness. "Covered" below means a lib module appears in at least one
such gate, not that its unit suite exists. Every lib module has its own
host unit suite regardless of this table.

C1-C5 are HAL-peripheral interleaves (no lib module under test). C6-C12
gate lib modules against the tick ISR or the serial stack. The
`rx-loopback` firmware carries no C number: it is the RX-wall harness leg
(polled USART RX path, host-driven), not a combination-matrix row.

## C-number to directory

| C | Directory | What it interleaves |
|---|-----------|---------------------|
| C1 | `tests/epic-combo-uart-ssp/` | USART, SSP, EEPROM under Timer2 ISR |
| C2 | `tests/epic-combo-multitimer/` | Timer0, Timer1, Timer2, USART dispatch ratios |
| C3 | `tests/epic-combo-adc-uart/` | ADC read path, Timer1 ISR, polled USART TX |
| C4 | `tests/epic-combo-rb-uart/` | PORTB change ISR, USART TX, Timer2 |
| C5 | `tests/epic-combo-eeprom-isr/` | EEPROM unlock sequence under Timer2 ISR (PIC18) |
| C6 | `tests/epic-combo-tick-serial/` | `lib/tick`, `lib/serial` |
| C7 | `tests/epic-combo-tick-settings/` | `lib/tick`, `lib/settings` |
| C8 | `tests/epic-combo-taskmgr-serial/` | `lib/taskmgr`, `lib/serial` |
| C9 | `tests/epic-combo-encoder-tick/` | `lib/encoder`, `lib/tick` |
| C10 | `tests/epic-combo-lcd-tick/` | `lib/lcd`, `lib/tick` |
| C11 | `tests/epic-combo-swuart-tick/` | `lib/swuart`, `lib/tick` |
| C12 | `tests/epic-combo-modbus-full/` | `lib/modbus`, `lib/serial`, `lib/tick` |
| - | `tests/epic-combo-rx-loopback/` | USART RX echo path (unnumbered harness leg) |

## Lib-module coverage

| Module | Gate | Note |
|--------|------|------|
| `lib/encoder` | C9 | Scripted quadrature under live tick ISR |
| `lib/serial` | C6, C8, C12 | Ring-buffered TX under tick ISR, scheduler, modbus stack |
| `lib/tick` | C6, C7, C9, C10, C11, C12 | The ISR side of every C6-C12 gate except C8 |
| `lib/swuart` | C11 | Bit-banged TX preempted by tick ISR |
| `lib/taskmgr` | C8 | Cooperative scheduler over interrupt-driven serial |
| `lib/modbus` | C12 | Full RTU stack over serial and tick |
| `lib/settings` | C7 | EEPROM settings under tick ISR |
| `lib/lcd` | C10 | HD44780 print sequence under tick ISR |
| `lib/adcfilter` | None | Pure computation over a caller-supplied read callback, no ISR-owned state. |
| `lib/bus` | None | Synchronous register transactions over polled SSP, no ISR-owned state. |
| `lib/console` | None | Polled line dispatcher over the serial receive path gated by C8. |
| `lib/debounce` | None | Pure timebase consumer via `epic_tick_get`, no ISR critical section of its own. |
| `lib/fsm` | None | Pure control-flow tables, no hardware or ISR interaction. |
| `lib/math` | None | Stateless fixed-point arithmetic, no ISR interaction. |
| `lib/mcp23x17` | None | Synchronous expander driver over `lib/bus`, no ISR-owned state. |
| `lib/pid` | None | Pure arithmetic stepped by the caller each period, no ISR interaction. |
| `lib/sdcard` | None | Blocking 512-byte SPI block driver, polled, no ISR-interleave design point. |
| `lib/usb` | None | CDC-ACM stack on the PIC18 SIE, outside the USART and Timer ISR scope of these gates. |

Trace: each C row's `@brief` header names its number (e.g. C9 in
`tests/epic-combo-encoder-tick/tests/combo_encoder_tick.c`) and each
`depends_on` entry in `common/manifest/modules.toml` names the lib modules
linked into that gate. The include scan agrees: only `encoder.h`,
`epic_serial.h`, `epic_tick.h`, `epic_swuart.h`, `epic_taskmgr.h`,
`epic_modbus.h`, `epic_settings.h`, `epic_lcd.h` and `epic_lcd_transport.h`
(the C10 transport shim) appear under `tests/epic-combo-*/tests/*.c`.
