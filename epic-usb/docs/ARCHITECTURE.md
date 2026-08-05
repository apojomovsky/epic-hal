# `epic-usb` architecture

USB CDC-ACM (virtual serial port) device for PIC18F2455/2550/4455/4550,
wrapping the vendored M-Stack USB device stack.

## What it is

`epic-usb` adds ring-buffered, non-blocking `write`/`read`/`available`/`flush`
plus a DTR-gated `connected()` on top of M-Stack's raw endpoint-buffer API.
M-Stack handles USB enumeration, control transfers, and the CDC class driver;
this module owns the byte-stream interface firmware actually calls.

## Why this module breaks the family-agnostic pattern

Every other `epic-*` module is either genuinely vendor-agnostic C or has a
per-family backend split with a portable-C reference. `epic-usb` is neither:
M-Stack's `usb_hal.h` talks directly to the PIC18F4550-family SIE (Serial
Interface Engine) and BDT (Buffer Descriptor Table), and PIC16F87XA parts
have no USB peripheral at all. There will never be a PIC16 backend. This
module is PIC18Fxx5x-only by hardware necessity, not by narrow initial scope.

## Ring buffers over raw endpoints

M-Stack has no `read()`/`write()` or ring buffer of its own for CDC — the
real API is raw endpoint access (`usb_get_in_buffer`, `usb_send_in_buffer`,
`usb_get_out_buffer`, `usb_arm_out_endpoint`, ...). `epic_usb.c` builds the
same ring-buffer discipline `epic-serial` uses on top of those primitives:

- **TX**: `epic_usb_service()` drains as many queued bytes as fit into one
  EP2 IN packet whenever the endpoint isn't busy. `write()` enqueues to the
  ring and calls `service()` once to kick a send immediately.
- **RX**: `epic_usb_service()` copies every byte from the OUT endpoint into
  the RX ring (dropping on overflow, same policy as `epic-serial`) and
  re-arms the endpoint.
- Ring size is a power of two (default 64 = one full-speed bulk packet),
  masked for O(1) index arithmetic. Override with
  `-DPIC8_USB_RING_SZ=128` before including the header.

## Connection state: DTR, not enumeration

`epic_usb_connected()` tracks DTR asserted via the CDC
`SET_CONTROL_LINE_STATE` request — not `usb_is_configured()`. A host can
enumerate the device and never open a terminal; `usb_is_configured()` would
be true either way. Only DTR means "a terminal opened the port." Output
gated on `connected()` waits until something is actually listening.

`epic_usb_reset_cb()` forces DTR false on bus reset, so stale state from a
previous connection doesn't survive a replug.

## Polling, not interrupts

M-Stack can be built in either polling or interrupt-driven mode. This module
uses polling exclusively: `epic_usb_service()` is called from the firmware's
main loop or a `epic-taskmgr` task. `USB_USE_INTERRUPTS` is deliberately
left undefined in `src/usb_config.h`. Two reasons:

1. The polling cadence (low single-digit milliseconds) is what M-Stack's own
   examples use and is well-understood.
2. An interrupt-driven mode adds an ISR-vs-main concurrency concern that the
   current ring-buffer code (no critical sections) does not account for.
   Adding it is not hard, but it's speculative complexity — build what's
   needed now.

## The M-Stack callback binding

M-Stack's `usb_config.h` convention requires the application to define macros
pointing to callback functions. This module names them `epic_usb_*_cb` (not
the `app_*` convention M-Stack's own demo uses) so they don't collide with a
firmware's own `app_*` callbacks if something else in the same build also
uses M-Stack directly. Most callbacks are correctly-typed no-ops; the two
that matter are `epic_usb_cdc_set_control_line_state_cb` (DTR tracking) and
`epic_usb_unknown_setup_request_cb` (routes CDC class requests into M-Stack's
own `process_cdc_setup_request`).

## Endpoint layout

- **EP1 IN**: CDC notification endpoint — present because the CDC-ACM
  interface descriptor requires it, but `epic_usb.c` does not send
  serial-state notifications on it.
- **EP2 IN/OUT**: Bulk data pipe — what `epic_usb_write`/`read` actually
  move bytes through.

## Host build boundary

The host build links `epic_usb_host_stub.c` — a **genuinely separate
implementation**, not a compile-time swap of `epic_usb.c`. This is honest
about what host tests can and cannot prove:

- **What host tests prove**: the public API's behavioral contract — ring
  fill/drain ordering, overflow-drop policy, `connected()`'s DTR-gated
  state transitions, `write()` draining once connected.
- **What host tests do NOT prove**: real USB enumeration, real endpoint
  buffer code paths, any interaction with a real host's driver stack.

There is no faithful host simulation of a USB SIE enumerating against a real
host, and no amount of host testing substitutes for plugging a board into a
Linux/Windows/macOS box and checking `dmesg`/Device Manager. The
`epic_usb_test_support.h` hooks (`epic_usb_test_set_dtr`,
`epic_usb_test_inject_rx`, `epic_usb_test_sent_*`) simulate just enough of
the host's side of the protocol to exercise the ring-buffer contract —
nothing more.

## Descriptor set

Adapted from M-Stack's own `cdc_acm` demo. The VID/PID pair (0xA0A0 /
0x0004) is M-Stack's bundled placeholder — fine for bench bring-up, **not
for redistribution**. Replace with a real [pid.codes](https://pid.codes)
allocation (VID 0x1209) before shipping a device. The serial number string
is a placeholder too ("DEV BUILD - NOT FOR SHIPPING."); a real deployment
needs a genuine per-unit serial (e.g. read from EEPROM) so a host doesn't
confuse two boards sharing the same dev VID/PID.
