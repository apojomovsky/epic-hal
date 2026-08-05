# `pic8-serial` architecture

Interrupt-driven, ring-buffered UART plus a `printf` retarget, the
non-blocking serial layer STM32Cube's `HAL_UART_Transmit_DMA`/`Receive_DMA`/
`_IT` gives, for 8-bit PICs.

## What it is

`pic8-serial` layers ring buffers and demand-driven interrupt TX/RX on the
HAL's USART driver. Received bytes land in the RX ISR and a ring; `read` pulls
from it without blocking. `write` enqueues to a TX ring and the TX ISR drains
it in the background, so the main loop is never blocked stuffing TXREG.
`putch` retargets XC8's `printf` to the TX ring. One family-agnostic source
builds against `pic16f87xa-hal` or `pic18fxx5x-hal` and on the host sim.

## How the ISRs are installed

Through the USART handle's `RxCpltCallback` / `TxCpltCallback`, the HAL owns
`USART_RX/TX_IRQHandler` (a **strong** definition on the XC8 target, where
`PIC8_WEAK` expands to nothing, so redefining them would be a
multiple-definition link error). The HAL's RX handler reads RCREG and calls
`RxCpltCallback(byte)`; its TX handler calls `TxCpltCallback()` when TXIF is
set. This module's callbacks push to / pop from the rings. This is the same
"set the callback, don't redefine the handler" pattern `pic8-taskmgr` uses for
its Timer0 tick.

The USART handle is `static`: the driver stores the caller's pointer
(`g_usart = h`), so a stack-local handle would dangle after `init` returns and
the callbacks would read stale memory. (Same lifetime rule as `pic8-tick`'s
Timer2 handle on PIC16.)

## TX is demand-driven; RX is always-on

`init` enables RCIE (RX always receives into the ring) but disables TXIE
right after, TX is idle until `write` enables it. The TX callback drains one
byte per TXIF and disables TXIE when the ring empties, so the idle TX ISR
does not fire. Ring access shared between ISR and main is critical-sectioned
(`EPIC_IRQ_Disable`/`Restore`); the single-byte ring counters are read
atomically, so `available`/`tx_pending` need no lock.

## The one family branch

The two USART peripherals differ in: the handle shape (PIC18 adds
`BaudGen`/`SPBRGH`), `USART_ComputeSPBRG` (PIC18 takes a BRG16 arg), the
TX/RX IRQ numbers, and the TXREG write (PIC16 `PIC8_REG8 =`, PIC18
`pic8_sfr_write8`, XC8 can't lower `|=` on a volatile cast lvalue at a
runtime SFR address). These are `#if`-gated on the family device define the
build already passes; everything else is family-neutral through `pic8_hal.h`.

## Host testing

The host sim models USART TXIF (re-asserted each sim step while TXEN) and RX
(via `*_sim_drive_usart_rx`, which sets RCIF and fires the dispatch). The host
test injects RX bytes and reads them back; for TX it writes a buffer, pumps
`pic8_dispatch_all_irqs` (the host analogue of the TX ISR firing) with TXIF
set, and captures what the TX callback loads into TXREG. The target demo
(`example_serial_target.c`) instead prints a banner and echoes RX→TX forever.
it uses no host-sim API, so the XC8 Makefiles build it.
