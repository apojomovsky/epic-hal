# #150: PIC18 family split design (beyond 18Fxx5x)

Status: approved by Main 2026-09-15 (D1-D5, phased reconciliation
included). Rulebook for #174/#175-#177, #178/#179-#181 (B-P1 landed
as #188), #182/#183-#185. Umbrella #150 stays open until all land.

Phase-1 tickets (#174 A, #178 B, #182 C) encode this split; this
note is the approval-gate record and the rulebook for the batches.

## Ground facts (verified, not assumed)

epic-cc master already carries all three exemplar TOMLs (tier atdf,
same PIC18Fxxxx DFP), so the HAL confirms against landed data:

- Same core contract everywhere: vectors [0x0008, 0x0018], stack 31.
  All three fork from pic18fxx5x-hal per adding-a-device.md section 5
  step 1 (closest by addressing model and interrupt architecture).
- Access bank: 2520/1320 0x0000-0x007F; 6520 0x0000-0x005F (equals 4550).
- RAM: 2520 0x0010-0x05FF; 1320 0x0010-0x00FF; 6520 0x0010-0x07FF
  (equals 4550). The 1320 is RAM-starved from the start: section 5
  step 6 storage caution applies at foundation, not later.
- Config: all three 13 bytes at 0x300001, no usbdiv/cpudiv/plldiv/
  vregen. Osc masks differ (2520 0x0F, 6520 0x07); 4550 is 14 bytes
  at 0x300000. Config handling stays per-family in the manifest.

## Decisions

D1: Three families, one exemplar each. pic18f2520-hal (A, 28/40-pin
mid-gen), pic18f1320-hal (B, 18-pin 1x20), pic18f6520-hal (C,
64/80-pin). Not absorbed into pic18fxx5x-hal (config layout, port
sets, and the no-USB shape differ; xx5x stays the 4-part USB family)
and not one mega-family (pin and peripheral shapes differ more than
the 63x multi-die precedent covers).

D2: Follow-up batches add variants INTO these families, never new
families, unless vectors or access-bank shape break. Delta mechanism
is the two proven patterns combined: per-part blocks with
FAMILY_* capability macros (63x precedent:
pic16f63x_67x_68x_hal.h) gating use sites, plus manifest
conditional_sources and per-variant config overrides (xx5x SPP
precedent). Family A spans legacy DS39564 and mid-gen 28/40-pin;
CAN (2480/2580/4580) and motor-control (23x1/43x1) parts join via
macros. Each batch re-confirms vectors/access-bank from that part's
TOML first; a break narrows D2 then, no preemptive fourth family.

D3: No driver crosses families without DFP-header confirmation.
SPP/USB do not carry over anywhere (no USB silicon in any of the
three). 6520 extra ports and 1320 peripheral set come from the DFP
headers, never from 4550 symmetry. Absence gets the same treatment
as 193X CCP4/5: grep the header, confirm non-existence, cite it.

D4: CI wiring in current terms. adding-a-device.md section 5 step 9
and its checklist name scripts/ci-discover-xc8-matrix.py, which no
longer exists (replaced by the manifest system). Until that doc is
corrected, foundation acceptance for each family is: modules.toml
family stanza plus modules slot, epic_build.py CANONICAL entry,
sfr-map-audit.py FAMILIES entry, sim-tests.yml pilot entry,
verified by running epic_build.py matrix and grepping for the
family. Correcting the doc references rides with the umbrella
close-out, not any single phase ticket.

D5: epic-cc side needs nothing (TOMLs landed). HAL scope is XC8 plus
mdb-SIM gates only, no board targets. 6520 stays XC8 plus aUDS.

## Open question (one)

Whether legacy 28/40-pin parts (242/248/252/258) truly fit Family A.
First Family A batch answers from TOMLs before adding them. No work
is premised on either answer.

## Lane state at note time

B-P1 (#178) is in review as PR #188: do not touch. A-P1 (#174,
claimed here) has an uncommitted prior-session tree resuming after
approval; its .scratch/ and probe files get triaged out before any
PR. C-P1 (#182) is unclaimed; claim follows approval, area
arbitration stays with Main.
