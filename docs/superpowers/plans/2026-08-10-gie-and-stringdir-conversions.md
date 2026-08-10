# GIE-race conversion (class G) + stringdir ISR-handler conversion (class F)

Status: **complete 2026-08-10**. Both conversions land (see the PR).
Task 3: all 14 ISR handlers use direct flag ops. Task 2: epic-serial
(all 3 sites), epic-swuart (3), epic-encoder (3), and epic-taskmgr
(7 of 10; task_start/task_reset/task_set_period retain their critical
sections with documented rationale - a 16-bit TCB write cannot be made
atomic by retry). Verification: 112/112 builds, 32/32 gates, 6/6
bundles, host suite green.

## Task 3: stringdir ISR-handler conversion (class F)

Replace the table-driven `EPIC_IRQ_GetFlag`/`EPIC_IRQ_ClearFlag` calls
in ISR-context handlers with direct flag ops on the literal register
tokens. The table reads route through XC8's stringdir/retlw path, which
clobbers PCLATH when the table sits on another flash page (the
documented latent hazard; the CCP1/CCP2 handlers already use the direct
pattern). The ISR path is bank-normalized (Finding 12), so the plain
direct reads are correct in ISR context. The public `EPIC_IRQ_GetFlag/
ClearFlag` API stays (main-line callers keep the table path); only the
ISR handlers convert.

Direct-flag pattern per handler:

```c
/* before */
if (!EPIC_IRQ_GetFlag(PIC16_IRQ_TMR2)) return;
EPIC_IRQ_ClearFlag(PIC16_IRQ_TMR2);
/* after */
if (!(EPIC_REG8(PIC_REG_PIR1) & PIC_PIR1_TMR2IF)) return;
EPIC_BIT_CLR(EPIC_REG8(PIC_REG_PIR1), PIC_PIR1_TMR2IF);
```

Flag mapping (ground truth from the irq_table in pic16_irq.c /
pic16f193x_irq.c; the third table field is in_intcon, the fourth is the
PIR index):

PIC16F87XA (10 handlers):
| Source | Register | Bit |
|---|---|---|
| RB (gpio.c) | INTCON | RBIF |
| TMR0 | INTCON | TMR0IF |
| TMR1 | PIR1 | TMR1IF |
| TMR2 | PIR1 | TMR2IF |
| SSP | PIR1 | SSPIF |
| USART TX | PIR1 | TXIF |
| USART RX | PIR1 | RCIF |
| ADC | PIR1 | ADIF |
| PSP | PIR1 | PSPIF |
| EEPROM | PIR2 | EEIF |
| CMP | PIR2 | CMIF |

PIC16F193X (4 handlers):
| Source | Register | Bit |
|---|---|---|
| IOC (gpio.c) | INTCON | IOCIF |
| TMR0 | INTCON | TMR0IF |
| TMR1 | PIR1 | TMR1IF |
| TMR2/4/6 (timer246.c) | PIR1/PIR3 | TMR2IF/TMR4IF/TMR6IF |

Files: pic16f87xa-hal timer0/timer1/timer2/ssp/usart/adc/eeprom/comp/
gpio/psp; pic16f193x-hal gpio/timer0/timer1/timer246.

## Task 2: class-G GIE-race conversion

Convert the remaining `EPIC_IRQ_Disable`/`EPIC_IRQ_Restore` sites to
lock-free or retry patterns. Finding 10.1: MPLAB SIM (and real-silicon
semantics) can deliver a latched interrupt inside a GIE=0 window,
tearing multi-byte reads and leaving GIE cleared after ISR return.
epic_tick_get's read-twice-retry is the reference.

Per-module analysis (each conversion's reasoning goes in the commit):

- **epic_serial**: remove the Disable/Restore from the TX ISR callback
  (an ISR-context disable protects nothing: the pop is the only ISR
  writer of the TX ring's tail/count, and the ISR cannot preempt
  itself on the single-vector families). The main-line push/read keep
  their Disable/Restore initially, then convert the multi-byte reads
  to read-twice-retry where the counters are multi-byte (verify: the
  counters are uint8_t single-byte increments, which are atomic on
  PIC16; the ring indexes likewise. If single-byte, the main-line
  Disables can be dropped too, with the ring discipline documented).
- **epic_swuart**: write/read/error-count sites. The ring push/pop are
  single-byte-indexed; the 16-bit error_count read is the multi-byte
  case (read-twice-retry).
- **epic_taskmgr**: 10 sites protecting TCB fields + the 16-bit tick
  counter. The 16-bit g_ticks read converts to read-twice-retry; the
  TCB field updates are single-byte or the site is a whole-struct
  write that only main does (verify per site; the ISR's tick write is
  the only ISR writer).
- **epic_encoder**: 32-bit position read (read-twice-retry), 16-bit
  error/glitch counts (read-twice-retry).

The encoder gate already hammers the 32-bit read under a live tick
(no tear observed); the conversion preserves that behavior without the
GIE manipulation.

## Verification

- Per-conversion gates: epic-serial, epic-swuart, epic-taskmgr,
  epic-encoder, epic-tick, and the combos that exercise these modules
  (C6 tick-serial, C8 taskmgr-serial, C9 encoder-tick, C11
  swuart-tick), plus the 193X firmware gate for the 193X handlers.
- Full make target-ci (112 builds, 32 gates, 6 bundles) + host suite.
- pre-commit checks clean.

## Out of scope

- Task 4 (host property/fuzz) and task 5 (layout hardening): separate
  PRs per the roadmap.
- The PIC18 handlers already use the switch-based direct pattern (no
  stringdir table); no conversion needed there.
