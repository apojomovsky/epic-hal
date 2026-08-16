# Soft rebrand: Epicurus -> Epic HAL

Ephemeral plan doc. Deleted when the work lands; git history is the archive.

## Goal

Rename the project from "Epicurus" to "Epic HAL" (repo `epic-hal`) across
docs, tooling, build artifacts, and paths. The C API does not move. This is
the first half of a family naming scheme whose second half is `epic-cc`, a
Rust+clang PIC compiler that will become a first-class backend alongside XC8.

## Invariant

The literal string `epicurus` appears in zero first-party C symbols
(verified: only the three `examples/*/main.c` header comments contain it).
Therefore the rename is token-scoped and cannot touch the ABI.

Explicitly unchanged:

- `EPIC_GPIO_Init` and every `EPIC_*` HAL contract symbol
- `epic_serial_init` and every `epic_x_*` module symbol
- `epic-*/` module directory names
- `EPIC_HAL_IRQ_H`, `EPIC_HAL_LCD_H` and every include guard
- family-internal prefixes `pic16f87xa_*`, `pic18_*`, `pic16f193x_*`

`EPIC_HAL_` is reused as the Make/env variable prefix. It shares a textual
prefix with the existing C include guards but collides with no actual name,
and Make and cpp are separate namespaces.

## Rename table

| Kind | From | To |
| --- | --- | --- |
| Display name | Epicurus | Epic HAL |
| Repo, URLs | `apojomovsky/epicurus` | `apojomovsky/epic-hal` |
| CLI command | `epicurus init` | `epic-hal init` |
| PyPI dist | `epicurus-cli` | `epic-hal-cli` |
| Python modules | `epicurus.py`, `epicurus_init.py` | `epic_hal.py`, `epic_hal_init.py` |
| Test module | `test_epicurus_init.py` | `test_epic_hal_init.py` |
| Bundle fragment | `epicurus.mk` | `epic-hal.mk` |
| Bundle manifest | `epicurus-sources.json` | `epic-hal-sources.json` |
| Make vars | `EPICURUS_*` | `EPIC_HAL_*` |
| Env vars | `EPICURUS_BASE_URL`, `EPICURUS_DIR` | `EPIC_HAL_BASE_URL`, `EPIC_HAL_DIR` |
| Vendor path | `third_party/epicurus/` | `third_party/epic-hal/` |
| Release assets | `epicurus-<family>-<ver>.tar.gz`, `epicurus-cli-<ver>.tar.gz` | `epic-hal-<family>-<ver>.tar.gz`, `epic-hal-cli-<ver>.tar.gz` |
| Examples, repo | `examples/epicurus-demo-<slug>.X` | `examples/epic-hal-demo-<slug>.X` |
| Examples, bundle | `examples/epicurus-demo.X` | `examples/epic-hal-demo.X` |
| Logo assets | `docs/assets/epicurus-logo-{dark,light}-mode.svg` | `docs/assets/epic-hal-logo-*.svg` |

Examples exist under two layouts: slugged in-repo, unslugged inside a
generated bundle. Both move, and the `FINAL_IMAGE` / `DISTDIR` strings inside
each `.X`'s `nbproject/*.mk` follow.

## Steps

1. `git mv` the 5 file/dir renames: 3 example `.X` dirs, 2 logo SVGs.
2. `git mv` the 3 Python modules.
3. Scoped textual pass over the 42 tracked files, longest token first so no
   prefix is half-eaten:
   `EPICURUS_` -> `EPIC_HAL_`; `epicurus_init` -> `epic_hal_init`;
   `test_epicurus_init` -> `test_epic_hal_init`; `epicurus.py` -> `epic_hal.py`;
   `epicurus-demo` -> `epic-hal-demo`; `epicurus-cli` -> `epic-hal-cli`;
   `epicurus.mk` -> `epic-hal.mk`; `epicurus-sources` -> `epic-hal-sources`;
   `epicurus-logo` -> `epic-hal-logo`; then bare `epicurus` -> `epic-hal` and
   `Epicurus` -> `Epic HAL`.
4. Human read of every prose hunk. `epic-hal` is wrong where a sentence wants
   "Epic HAL"; sed cannot make that call.
5. `pyproject.toml`: dist name, description, `[project.scripts]` entry point,
   `py-modules` list.
6. `AGENTS.md` (and its `CLAUDE.md` symlink): 3 references to the CLI, the
   script path, and `epicurus.mk`.
7. README: `<h1>`, logo `srcset`/`src`/alt text, all badge and release URLs.

## Decisions

- **Clean break, no compat shim.** Released 0.x bundles reference
  `epicurus.mk` and `EPICURUS_DIR`. `release-bundles.yml` already ships the
  "0.x, the interface may change" notice. No dual-named files, no alias vars.
- **The GitHub repo rename is the user's action**, not scriptable here.
  GitHub redirects the old path so published `curl | sh` lines keep
  resolving, but every URL in-tree moves to the new one regardless.
- **Logo art is kept**, files renamed, alt text updated. The laurel/temple
  iconography is Epicurean heritage that no longer matches the name; revisit
  as a family mark when `epic-cc` ships. Not this change.

## Out of scope

`epic-cc` integration. This lands the HAL side of the family namespace so the
compiler has a consistent scheme to arrive into. Wiring a second compiler
backend into `epic-hal.mk` is separate work.

## Verification

- `grep -ri epicurus` over the tree returns nothing outside `.git`.
- `python3 scripts/tests/test_epic_hal_init.py`, `test_bundlegen.py`,
  `test_ci_noncode.py` pass.
- Host-sim `cmake -B build && cmake --build build && ctest` on a sample of
  modules.
- `scripts/doxygen_doc_check.py` clean (no signatures change, but the Python
  renames touch docstringed files).
- Real-target XC8, `mdb`, and bundle-gate paths run only as far as the local
  toolchain allows. Report exactly what ran and what did not.
