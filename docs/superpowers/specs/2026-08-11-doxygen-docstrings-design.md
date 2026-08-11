# Doxygen-style function docstrings, whole project

Status: **approved 2026-08-11, not started**.

## Problem

The cleanup pass trimmed comments but left most functions without
argument/return documentation. The project needs a uniform, Doxygen
-compliant function docstring contract: every function documents its
arguments and return value, in a style that would satisfy doxygen
without installing it.

## Goal

Every first-party function (headers and .c statics) carries a Doxygen
-style block: `@brief` always, `@param` per argument with in/out
semantics in the prose (no `[in]`/`[out]` tags), `@return` for non-void
functions, longer `@details` only when the behavior is not obvious.
Enforced by a custom checker script, no doxygen dependency. AGENTS.md
gains the contract so it persists.

## Decisions (user-approved)

1. **Custom checker, no doxygen.** A Python checker
   (`scripts/doxygen_doc_check.py`) is the compliance definition: it
   scans first-party sources and fails on missing function docs,
   missing `@param` for a named parameter, missing `@return` for
   non-void, or `[in]`/`[out]` tags. No Doxyfile, no CI dependency on
   doxygen.
2. **Coverage split.** Library code (all modules, the three HALs,
   epic-common): full `@brief`/`@param`/`@return` on every function,
   headers for public API declarations, `.c` for `static` functions
   (non-static `.c` definitions are covered by their header).
   Tests and examples: `@brief` on every function; `@param`/`@return`
   only where the function is non-trivial (CHECK-wrapper noise
   skipped). The checker has a brief-only mode for those paths.
3. **Format** (the contract):

```c
/**
 * @brief One-line summary.
 *
 * Longer explanation only when the behavior is not obvious.
 *
 * @param name what it is
 * @param out_buf where results are written
 * @return what the caller gets
 */
```

   - `@brief` required on every function doc block.
   - `@param name` per named parameter, order-insensitive, names must
     match the signature; no `@param[in]`/`@param[out]`/`[in,out]`.
   - `@return` required for non-void; no `@return` expected for void.
   - The doc block must be a Doxygen block (`/** ... */`), not `/*`
     or `//`, for functions.
   - No `@file`/`@author`/`@date` boilerplate (the cleanup's rules
     stay); `@details` only when needed.
4. **Existing comments are kept, not reduced.** Docstrings are added
   or converted; the previous cleanup's comment content stays. Where a
   function already has a comment block, it is converted to the
   Doxygen form and extended with the missing `@param`/`@return`
   lines, not rewritten from scratch.
5. **AGENTS.md** gains a "Function documentation" subsection under
   Expression conventions stating this contract, so future work keeps
   it.

## The checker (`scripts/doxygen_doc_check.py`)

- Scans a file list or first-party tree; per file extracts C function
  declarations/definitions and their immediately preceding doc
  comments with a small tokenizer (balanced-paren parameter parsing,
  handles pointers, `static`, multi-line signatures, `const`,
  variadic `...`).
- For each function: doc block present and `/**`-style; `@brief`
  present; every named parameter has a matching `@param`; non-void has
  `@return`; no `[in]`/`[out]` anywhere in the block.
- Modes: `--brief-only` for tests/examples paths (requires `@brief`,
  does not require `@param`/`@return`).
- Fail-closed: constructs it cannot parse (e.g. exotic function
  pointers) are reported as unverifiable, not silently skipped, and
  exit nonzero so a human looks.
- Own unit tests in `scripts/tests/test_doxygen_doc_check.py`,
  following the existing pattern.
- Exit 0 = compliant; per-module runs gate each batch; a full-tree run
  is the final gate. No CI changes in this work (the checker can be
  wired into CI later if wanted; not part of this plan).

## Scope

- All first-party `.c`/`.h` under: pic16f87xa-hal, pic18fxx5x-hal,
  pic16f193x-hal, epic-common, and every epic-* module.
- Tests (`*/tests/*`, `*/examples/*`, `examples/`) get brief-only.
- Third-party (`epic-usb/third_party/**`) untouched.

## Phases

- **Phase 0** : The checker + its unit tests (Task 1).
- **Phase 1** : AGENTS.md conventions update (Task 2).
- **Phase 2** : Docstring batches, one per module group (the same 11
  groupings as the cleanup), each gated by the checker (full mode for
  the module's lib code, brief mode for its tests/examples) + the
  module's host ctest.
- **Phase 3** : Final gate: full-tree checker run (exit 0), full
  22-module ctest sweep, pre-commit checks.

## Verification

- `python3 scripts/doxygen_doc_check.py <file>` reports no violations
  for every instrumented file; the full-tree run exits 0.
- `python3 scripts/tests/test_doxygen_doc_check.py` passes.
- 22-module host cmake+ctest sweep PASS (docstrings only, no
  semantics change).
- Pre-commit checks PASS.
- Spot review: every function in a sample of each module carries the
  contract.

## Out of scope

- Doxygen installation or a Doxyfile (user chose the custom checker).
- CI changes (the checker is not wired into workflows in this work).
- Any comment reduction or behavior change.
- Third-party code.
