# 628A on shared pic14-midrange-core (epic-hal#136)

## Goal
Full PIC16F628A subset parity on a new shared `pic14-midrange-core/`,
deduping 87XA/88X. Next subset part costs SFR + irq map + manifest +
tests only. No 87XA/88X behavior change.

## Names (agreed)
Cores are ISA units: `pic-baseline-core` (future), `pic14-midrange-core`
(new, this ticket), `pic14-enhanced-core` (future, 193X migrates later),
`pic18-core` (future). Manifest families stay part-grouped (`PIC16F628A`
new). Macros: `PIC14MIDRANGE_HAS_*`. 628A: closest sibling 877A, Path A
(RP0/RP1 banking, single vector 0x0004, DS40044G).

## Mechanics: neutral headers (platform.h precedent)
Shared `.c` files MUST NOT name device headers. Pattern, per file moved:
- Declarations move to `pic14-midrange-core/include/<same-subdir>/pic14_<periph>.h`
  (neutral names; APIs already identical across families per fixed contract).
- Per-family `pic16f87xa_<periph>.h` / `pic16f88x_<periph>.h` become thin
  redirects (`#include` the neutral header) + feature macros. Public paths
  stay stable for consumers.
- SFR access: shared `.c` includes neutral `pic14_midrange_sfr.h`, provided
  per family (each family's copy `#include`s its generated
  `pic16fXXXX_sfr.h`). Same trick as host/target `platform.h` selection.
- Sim hooks (`pic16f87xa_sim_*` vs `pic16f88x_sim_*`): rename to neutral
  `pic14_sim_*` in both sim backends (one function each, trivial).
- Headers/sources listed in manifest `hal_sources` + family `CMakeLists.txt`
  `EPIC_SOURCES` by their new paths (arbitrary paths already supported).

## Wave 1 (verbatim, no logic change)
Move: eeprom.c(266), timer0.c(~105), timer2.c(~135), vref.c(~68),
harness sim/mdb, tiers_inc.h, wdt sim/target/epiccc twins, isr_vector.c,
plus their headers per mechanics above. Redirects in both families.
Verify: 87XA + 88X host build+ctest green, real-target matrix unchanged.

## Wave 2 (flag parameterization)
irq table -> per-device `pic16fXXXX_irq_map.h` (`irq_desc_t` stays shared);
dispatch fan-out + usart(TX/SPBRGH/autobaud/break)/ssp-mask/gpio(ANSEL,
WPUB/IOCB, port widths)/timer1-gate/ccp-ECCP behind `PIC14MIDRANGE_HAS_*`
(PSP precedent). Sim skeleton shared, model fns per device.

## Wave 3 (628A shim)
`pic16f628a-hal/` thin shim: generated SFR (`gen-sfr.py --family PIC16F628A`,
new choice, `--check` gate), platform headers, irq map (no PIR2/CCP2/SSP/
ADC), port table (A/B only, no ANSEL), 128B eeprom path (no EEDATH/H),
8-bit usart, PORTC/D/E = `#error`. Manifest `[families.PIC16F628A]`
(variants 16F628A, epiccc slice gpio+timer0+timer2+usart+core+harness, no
SSP) + per-module supported/excluded (expect excluded: 224B RAM smallest)
+ reference .X + CI legs (family job, run_one pilots, epiccc leg).

## Verification (adding-a-device sect. 4, mandatory)
Every 628A peripheral: host test with hand-computed register image, then
real-target XC8 build + mdb `stepi` register readback. Full regression:
every module x every variant, host+target, 87XA+88X+628A. `gen-sfr --check`
green. MANUAL.md + README gotchas updated.

## Risks
Runtime SFR addresses + bank-switch adjacency (sect. 4.9); CMCON bank
suspect on 628A (settle via DFP diff); RAM budget (224B forces excluded
entries); 88X bank-3 SFRs (ANSEL/BAUDCTL) must stay guarded out of 628A.
