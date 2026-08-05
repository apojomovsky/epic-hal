# `epic-sdcard` architecture

SD/MMC-over-SPI block storage for PIC18F2455/2550/4455/4550, wrapping the
vendored M-Stack storage driver (`mmc.c`/`crc.c`).

## What it is

`epic-sdcard` binds M-Stack's portable `mmc.h`/`crc.h` to this repo's SSP
HAL (`EPIC_SSP_Init`/`WriteByte`/`ReadByte`), GPIO HAL
(`EPIC_GPIO_WritePin`), and `epic-tick` (real wall-clock timeouts). The
public API is thin call-throughs to `mmc_init_card`/`mmc_read_block`/
`mmc_write_block`; the real content is the binding layer.

## Why PIC18Fxx5x-only — RAM, not the peripheral

Both families have a working SSP driver with the same neutral API. The
blocker is `MMC_BLOCK_SIZE` — a fixed 512 bytes, the SD protocol's only
supported block size. Every PIC16F87XA family member's total RAM (192 B for
PIC16F873A/874A, 368 B for PIC16F876A/877A) is smaller than one block. Even
the largest part falls 144 bytes short before accounting for the `mmc_card`
struct, CRC state, the call stack, or anything else the caller's firmware
needs.

The binding layer itself (`epic_sdcard.c`) is honestly portable C — no
family-specific SFRs, no inline asm, only HAL calls already proven
family-agnostic. If this repo ever gains a PIC18 or PIC16 family member with
enough RAM, the module can target it without rewriting `epic_sdcard.c`.

## The binding layer

### SPI byte transfer

`epic_sdcard_spi_transfer` implements `MMC_SPI_TRANSFER` by calling
`EPIC_SSP_WriteByte` in a loop (with write-collision retry: the SSP's WCOL
bit must be cleared in software per DS39582E §19.2.2), then
`EPIC_SSP_ReadByte` for the response. When `out_buf` is NULL it clocks out
0xFF (SD-over-SPI junk while reading); when `in_buf` is NULL it discards the
response.

### CS pin control

`epic_sdcard_spi_set_cs` asserts/deasserts the card's CS line via
`EPIC_GPIO_WritePin`. The caller supplies the port/pin at init time. CS is
ordinary GPIO, different boards wire it differently, and the SSP peripheral
only owns SCK/SDI/SDO.

### SPI clock speed

`epic_sdcard_spi_set_speed` picks the fastest of the SSP's three fixed SPI
divisors (Fosc/4, /16, /64) that does not exceed the requested rate, then
re-initializes the SSP at that divisor. If none meet the target, it falls
back to the slowest available rather than silently picking something faster.

**Known gap**: at Fosc = 48 MHz (the USB-mandated clock), Fosc/64 = 750 kHz,
which cannot reach the SD spec's mandatory ≤ 400 kHz bring-up speed. The
SSP's fixed divisors only get there at Fosc ≤ 25.6 MHz. Reaching true spec
compliance at 48 MHz would need the SSP's TMR2/2 clock source
(SSP_MODE_SPI_MASTER_TMR2, arbitrary divisor via Timer2's PR2), not
implemented here. Many real cards tolerate a faster-than-spec bring-up clock
in practice, but this is unverified without real hardware.

### Timeouts via epic-tick

`MMC_USE_TIMER` is defined in the target's `mmc_config.h`, binding the three
timer macros to `epic-tick` (`epic_sdcard_timer_start`/`expired`/`stop`).
This activates M-Stack's own spec-derived timeout constants
(`MMC_READ_TIMEOUT` 150 ms, `MMC_WRITE_TIMEOUT` 500 ms) — strictly better
than the retry-count fallback (32768 ACMD41 retries with no wall-clock
bound) that `mmc.c` uses when `MMC_USE_TIMER` is undefined.

The host test build deliberately does **not** define `MMC_USE_TIMER` — the
mock SPI slave has no real latency to time out against, so the bounded-retry
fallback is both simpler and sufficient.

## The mock SPI slave

`tests/mock/epic_sdcard_mock_spi.c` plays the SD card's side of the SPI
command/response protocol (CMD0/8/55/41/58/9/16/17/24/13) against a small
4-block in-memory backing store. The card *reports* 1024 blocks via a
crafted SDHC-style CSD v2.0 register (matching a real card's response
shape), but only the first 4 blocks are actually backed — enough for
read/write/independence tests without allocating hundreds of kilobytes of
host RAM.

This is a **stronger host-testing story** than `epic-usb`'s host stub.
Because `mmc.c`/`crc.c` are genuinely portable C with no hardware
dependency (no SFRs, no interrupts, no compiler-specific pragmas), the host
build compiles the **actual vendored source** against the mock instead of
real hardware. Host tests exercise `mmc_init_card`/`mmc_read_block`/
`mmc_write_block`'s real protocol logic, not a hand-written approximation
of it.

### The CRC16 byte-order correction

The mock required one non-obvious fix during development: `mmc.c`'s
`__read_data_block` self-check (continue the CRC16 accumulator with the
trailing CRC bytes, expect 0) only holds when the trailing bytes are in
MSB-first (`[high, low]`) order. `mmc_write_block` sends them in
LSB-first (`[low, high]`) order — but that send is never self-checked by
anything in `mmc.c` (writes are validated by the card's data-response token
instead, not a self-check), so its byte order is simply an unrelated,
unverified convention. The mock (playing the *card*, i.e. the read path)
must send MSB-first.

## Host build story

The host build compiles `mmc.c`/`crc.c` against `tests/mock/mmc_config.h`
(which binds `MMC_SPI_TRANSFER` to the mock slave), plus the mock itself.
`epic_sdcard.c` is NOT compiled for the host build — it depends on
`epic_hal.h`/`epic-tick`, which need a real (or host-simulated) HAL family
with a meaningfully behaving SSP behind it. The mock SPI slave is enough to
test `mmc.c`'s own logic; testing the HAL binding layer is a real-target
concern.

## RAM headroom

`mmc.c` + `crc.c` alone measured 919 bytes RAM (44.9% of 2048) on real
PIC18F4550 headers. The gap versus the naive "just the 512-byte block buffer"
expectation is `mmc.c`'s internal call-graph-allocated locals under XC8's
stack-less auto-variable model — every function's locals get permanent RAM
allocation shaped by the call graph, not a transient stack frame.

The full integrated link (`epic_sdcard.c` + `mmc.c`/`crc.c` + the HAL +
`epic-tick`) did not complete in three XC8 attempts — likely the
auto-variable bank-packing search under real memory pressure rather than a
code error (every piece compiles clean individually). Combining with
`epic-usb` (454 bytes alone) in one firmware image is tighter still and
unproven.

## Multi-block write: reachable but not wrapped

`mmc.h` supports multi-block write (`mmc_multiblock_write_start/data/end`,
plus `_cancel`). `epic_sdcard.h` does not wrap it — no concrete caller
exists yet. The functions are reachable through the vendored `mmc.h` directly
if needed, and the linker's own "pointer may have no targets" advisories for
them suggest they may be excludable from the link to shrink the call graph if
RAM is tight.
