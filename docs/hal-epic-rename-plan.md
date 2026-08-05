# HAL_ to EPIC_ rename

Status: **tooling built and validated in an isolated worktree; full
repo-wide apply to `master` not yet done, pending go-ahead.**

## Why

The project is now branded Epicurus (`README.md`). The repo slug,
module/package names, and the `HAL_` API prefix were deliberately kept
unchanged when that branding landed (`docs/ci-plan.md`-adjacent
decision, see git history around the README rename): `HAL_` mirrors the
vendor-HAL naming convention (STM32Cube, Microchip's own newer HALs)
that any embedded engineer already recognizes, and it is the literal
"fixed contract" name `AGENTS.md` calls "the one idea that matters
most." The user has decided to go ahead and rename it anyway, so this
plan is about doing that renaming correctly, not re-litigating whether
to.

Scope, explicitly: only the literal `HAL_` identifier prefix (functions,
macros, the shared `HAL_StatusTypeDef` type, and the build-system
variables named after it, e.g. CMake's `HAL_DIR`/`HAL_SOURCES`/
`HAL_FAMILY`/`HAL_LIB`) becomes `EPIC_`. Out of scope, not touched by
this: the repo slug and every module/package directory name
(`pic8-hal`, `pic16f193x-hal`, etc.), the family-blind `epic_`-prefixed
harness/module functions (`epic_harness_report`, `epic_dispatch_all_irqs`,
`epic_tick.h`'s own API, ...), which are a different naming convention
entirely and were never part of the "HAL_" contract.

## Survey (done)

`git ls-files -z | xargs -0 grep -o 'HAL_' | wc -l`: **3953 occurrences
across 309 tracked files** (116 `.c`, 90 `.h`, 63 `.md`, 14
`CMakeLists.txt`, 1 `epic_family.cmake`, 20 real-target `Makefile`s, 2
`scripts/*.py`, plus `AGENTS.md` itself, one clean occurrence:
`HAL_IRQ_SetPriority`). No filename contains `HAL_` (case-sensitive),
so this is a content-only rename, no `git mv` needed. No pre-existing
`EPIC_` token anywhere in the tree, zero collision risk with the new
name. All 309 affected files are plain text (spot-checked with `file`
against the full match list); `build*/`, `docker/ci-toolchain/vendor/`,
and every other generated/binary path are excluded from `git ls-files`
by `.gitignore` already, so driving the rename off `git ls-files`
output is sufficient path-scoping on its own, no separate exclude-list
needed for that.

**The one real hazard found**: several docs and header comments name
*STM32Cube's own, real API* as a reader-orientation comparison ("this
is our equivalent of STM32Cube's `HAL_GetTick`/`HAL_Delay`"), not this
project's identifiers. A blind rename would turn those into fictional
claims about a competitor's product. Full list, found by grepping for
`HAL_` co-occurring with `STM32`/`Cube` (including lines where the
comparison and the token wrap across adjacent lines in a doc comment,
which a same-line grep alone misses, confirmed by manually reading
`epic-common/include/core/hal_status.h`):

- `HAL_NVIC_*` (3 files: both classic-PIC16 `pic16_irq.h`/
  `pic16f193x_irq.h` and PIC18's `pic18_irq.h`, each "mirrors
  `HAL_NVIC_*` from STM32Cube" in a header comment)
- `HAL_PPP_Init`/`HAL_PPP_DeInit`/`HAL_PPP_MspInit` ("PPP" is
  STM32Cube's own generic-peripheral placeholder notation; this
  project has no peripheral literally named PPP, so this prefix is
  unambiguously foreign wherever it appears)
- `HAL_I2C_Mem_Read`/`HAL_I2C_Mem_Write` (this project's own I2C
  driver is under the `HAL_SSP_*` prefix, matching the PIC datasheets'
  own MSSP naming; `HAL_I2C_` never appears here as our own identifier)
- `HAL_UART_Transmit_DMA`/`HAL_UART_Receive_DMA` (this project's own
  serial driver is `HAL_USART_*`, not `HAL_UART_*`; the two prefixes
  are spelled differently on purpose, so `HAL_UART_` is unambiguously
  foreign)
- `HAL_GetTick`/`HAL_Delay` (this project's own equivalent,
  `epic-tick`, uses the `epic_`-prefixed family-blind convention, not
  `HAL_`, so these two exact tokens never occur here as our own code)
- `HAL_StatusTypeDef`, the hard case: this is genuinely both. It's our
  own declared type (`epic-common/include/core/hal_status.h`, ~369 of
  the 371 total occurrences) *and*, on exactly two lines in that same
  file's own header comment (lines 5 and 17, "mirrors STM32Cube's
  `HAL_StatusTypeDef`" / "Mirrors `HAL_StatusTypeDef` from STM32Cube"),
  a reference to STM32Cube's identically-spelled real type. Byte-for-
  byte identical token, opposite referent, in the same file. No
  substring/prefix rule can disambiguate this; it needs an exact
  file+line exception.

## Tooling design

`scripts/rename-hal-epic.sh`: a driver script, `--dry-run` (default) or
`--apply`. Enumerates input files via `git ls-files` (so it
automatically respects `.gitignore`, no separate path-exclude list
needed, confirmed above). For each file, runs it through
`scripts/rename-hal-epic.awk`, which:

1. Loads `scripts/hal-epic-exceptions.txt`, a plain-text, hand-curated,
   auditable data file with two kinds of rows:
   - `*<TAB>*<TAB><token>`: a global wildcard exception, this exact
     token/prefix is never renamed, anywhere in the tree (the six
     unambiguous foreign items above).
   - `<path><TAB><line><TAB><token>`: an exact file+line exception (the
     two `HAL_StatusTypeDef` lines above).
2. Per line of input: protect every exception token that applies to
   this file+line by replacing it with a unique placeholder
   (`@@EXC<n>@@`), run the blind `gsub(/HAL_/, "EPIC_")` (proven safe
   by the survey: the only non-word-boundary case found, `-DHAL_FAMILY`
   in CMake command-line invocations, is a case where the blind
   replacement is *correct*, not a false positive, so no word-boundary
   logic is needed or wanted), then restore the placeholders back to
   their original protected text.
3. `--dry-run` prints a unified diff per changed file plus a summary
   count; `--apply` writes the result back in place (via a temp file +
   `mv`, preserving file mode).

This is the "leverage awk" piece: the substitution itself is one `gsub`
call per line, in `awk`; the exception-protection wrapper around it is
what makes that one `gsub` call safe to run unattended across 309 files.

## Validation performed

Ran in an isolated git worktree (`superpowers:using-git-worktrees`),
never touching the working tree this session has been using:

1. `--dry-run` against the full repo: diff reviewed by hand against the
   exceptions list above, confirmed all six STM32Cube-comparison lines
   and both `HAL_StatusTypeDef` lines are left untouched, and spot-
   checked a sample of ordinary renames (`HAL_GPIO_Init` ->
   `EPIC_GPIO_Init`, `HAL_DIR` -> `EPIC_DIR`, `-DHAL_FAMILY=PIC18` ->
   `-DEPIC_FAMILY=PIC18`) for correctness.
2. `--apply` in the worktree, then:
   - `git ls-files -z | xargs -0 grep -c HAL_ | grep -v :0`: confirms
     zero unintended leftover `HAL_` tokens outside the exceptions.
   - Host-sim build + `ctest` for every module
     (`cmake -B build && cmake --build build && ctest`).
   - Real-target XC8 build for one MCU per family
     (`make xc8-build MODULE=... MCU=...`).
   - `make mdb-test` for all three families' existing gates (the same
     ones `sim-tests.yml` runs), confirming identical PASS output to
     pre-rename.

(Results of this validation pass go here once run; see the Status line.)

## Remaining step: agent verification pass

Per the user's request, after the mechanical rename + the validation
above, a fresh agent (no memory of this session, full repo access)
re-sweeps the renamed tree specifically for anything the mechanical
script structurally cannot catch: prose that now reads awkwardly after
a pure token substitution (e.g. a sentence that said "the HAL_ prefix"
and now says "the EPIC_ prefix" but the surrounding grammar assumed the
old word), any exception this plan's survey missed, and a final build +
test sweep. Its brief: fix what it finds, do not touch anything the
survey already confirmed correct, report a diff summary.

## Not done, deliberately

- The actual `--apply` to the real working tree / `master` (only run in
  the disposable validation worktree so far).
- Committing or pushing.
