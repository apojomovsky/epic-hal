# `epic-lcd` architecture

HD44780-compatible character LCD driver with configurable transport, built
on the 8-bit PIC HAL.

## What it is

`epic-lcd` implements the HD44780/ST7066 command set (the controller inside
the Vishay LCD-016N002B-CFH-ET and virtually every character LCD) on top of
an injectable transport ops struct. The core logic never touches a pin or
SPI peripheral — it calls `ops->send(rs, byte)` and `ops->delay_us/ms()`,
and the transport does the rest.

## The transport ops seam

```c
typedef struct {
    void (*send)(void *ctx, uint8_t rs, uint8_t byte);
    void (*delay_us)(void *ctx, uint32_t us);
    void (*delay_ms)(void *ctx, uint32_t ms);
} epic_lcd_ops_t;
```

`send` writes one byte with RS selecting instruction (0) vs. data (1). The
4-bit GPIO transport splits the byte into two nibble-sends with two E pulses;
the 8-bit transport writes all 8 bits in one E pulse; the SPI transport
shifts out to a 74HC595 and latches three times per nibble. The core
doesn't know or care which one is in use.

The ops seam also enables host testing: the mock transport records every
command for assertion without real hardware.

## Why two headers

`epic_lcd.h` contains only HAL-independent types (`epic_lcd_ops_t`,
`epic_lcd_config_t`, `epic_lcd_t`, function declarations). The host test
build links against it without any HAL.

`epic_lcd_transport.h` adds the HAL-dependent pin structs
(`epic_lcd_gpio4_pins_t`, `epic_lcd_gpio8_pins_t`, `epic_lcd_spi_config_t`)
and their init functions. Target firmware includes both; the host test
includes only the first.

## Timed delays instead of busy-flag polling

R/W is tied low in all shipped transports, so the busy flag (BF) in the
instruction register is unreadable. Instead, every command is followed by
a delay matching its datasheet-specified execution time:

- Clear Display / Return Home: 1.6 ms
- All other commands: 40 us
- Init: 50 ms power-on wait, 4.5 ms between init writes

On target, `delay_ms` wraps `epic_tick_delay_ms`. Sub-millisecond delays
are approximate (the HAL's tick resolution is 1 ms); for the HD44780's
~40 us command time, the overhead of the GPIO/SPI calls themselves at
20-48 MHz already exceeds this minimum, making an explicit delay
unnecessary for most commands.

## 4-bit GPIO transport and the init sequence

The HD44780's 4-bit mode has a notoriously subtle init sequence: before the
controller knows it's in 4-bit mode, only the high nibble of each Function
Set command is interpreted. The datasheet specifies sending 0x3 (high nibble
of 0x3X) three times, then 0x2 to switch to 4-bit.

`epic_lcd_init()` sends full 8-bit-form Function Set commands (0x38). The
4-bit transport's `send()` splits every byte into two nibble-writes with
two E pulses. For the init, this produces:

1. First 0x38: nibble 0x3 + E pulse, nibble 0x8 + E pulse.
   The LCD (still in 8-bit mode) interprets only the first nibble 0x3.
2. Second 0x38: same thing. LCD processes 0x3 again.
3. Third 0x38: same. LCD is now stabilized.
4. After init, subsequent commands like Display ON/OFF (0x0C) are properly
   split into two nibbles by the transport, and the LCD interprets both
   nibbles as one complete 4-bit command.

This matches the HD44780 datasheet's 4-bit init procedure exactly, without
requiring the core to know whether it's talking 4-bit or 8-bit.

## SPI transport via 74HC595

The 74HC595 is an 8-bit shift register with output latch. Wiring one to an
HD44780 in 4-bit mode uses 6 of the 8 outputs (DB4-DB7, RS, E; R/W is
optional). A configurable `epic_lcd_spi_layout_t` maps which Q output
connects to which LCD signal.

The E-pulse procedure for one nibble:

1. Shift out a byte with the data nibble set, RS set, E=0. Latch.
2. Shift out the same byte with E=1. Latch. (This is the write strobe.)
3. Shift out with E=0 again. Latch. (End of strobe.)

Each nibble write requires three SPI transactions. A full byte (two nibbles
in 4-bit mode) requires six. At Fosc/64 SPI clock (750 kHz at 48 MHz),
one transaction takes ~11 us, so a full byte takes ~66 us — well within
the HD44780's execution time.

`EPIC_LCD_SPI_LAYOUT_COMMON` provides the most common wiring
(Q0=DB4, Q1=DB5, Q2=DB6, Q3=DB7, Q4=RS, Q5=E).

## Host testing

The mock transport (`tests/mock/epic_lcd_mock_transport.c`) records every
`send()` call with its RS state and byte value. Tests verify:

- Init sequence: three Function Sets, Clear, Entry Mode Set, Display ON
- DDRAM addressing: row 0 at 0x00, row 1 at 0x40, custom row addresses
- Display/cursor/blink control: correct bit manipulation in Display Ctrl
- Scroll commands: S/C and R/L bits
- Custom character: CGRAM address calculation, 8 data bytes, DDRAM return
- Boundary conditions: slot > 7 clamped, row bounds checked

What host tests do NOT prove: whether the display actually lights up,
whether E-pulse timing is met on real hardware, or whether the 74HC595
wiring is correct. That needs real silicon.

## Instance model

One `epic_lcd_t` per display. Multiple independent LCDs each get their own
ops, config, and cached state (display_ctrl, entry_mode). This follows the
same pattern as `debounce_t` (one per input) and `epic_adcfilter_avg_t`
(one per filter) in this repo.
