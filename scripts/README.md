# Dev environment scripts

Covers the host-toolchain bootstrap and pre-commit hook below. For
real-target XC8 builds and the `mdb` (MPLAB SIM) verification gate
without installing XC8/MPLAB X yourself, see the root
[`Makefile`](../Makefile) and [`docs/docker-dev-plan.md`](../docs/docker-dev-plan.md)
instead, everything runs inside a Docker image built from installers you
drop in `docker/ci-toolchain/vendor/`. `scripts/sim-mdb-run.sh` and
`scripts/sim-test-local.sh` (this directory) are the lower-level pieces
that flow reuses; their own header comments cover direct use if you
need it.

## Bootstrap

```sh
./scripts/bootstrap.sh              # install missing host packages + the git hook
./scripts/bootstrap.sh --check-only # report what's missing, install nothing,
                                     # exit nonzero if anything is missing
```

One-time (idempotent) setup for a fresh clone: installs the host toolchain
the CMake builds need (`cmake`, `build-essential`, `cppcheck`,
`clang-format`, via `apt-get`, so Debian/Ubuntu; other package managers
get a printed package list instead of an auto-install), then runs
`install-git-hooks.sh` below. Also checks whether `xc8-cc` is on `PATH`
and points at the README if it isn't. Real-target (XC8) builds need
MPLAB X + MPLAB XC8 v3.x installed manually (proprietary, license-gated,
an interactive installer, not something this script attempts); host
builds work fine without it.

## Pre-commit hook

Installed on its own, or as part of `bootstrap.sh` above:

```sh
./scripts/install-git-hooks.sh
```

This symlinks `.git/hooks/pre-commit` to `scripts/pre-commit-checks.sh`
(`.git/hooks/` isn't tracked by git, so every clone needs to run the
installer once). Uninstall by deleting `.git/hooks/pre-commit`, or skip it
for one commit with `git commit --no-verify`.

### What it checks

1. **Trailing newline / trailing whitespace.** Auto-fixes the working-tree
   file, then blocks the commit and asks you to `git diff`, review, and
   `git add` again. Never silently changes what gets committed without you
   seeing it. If you staged only part of a file with `git add -p`, this
   still edits the whole working-tree file, review before re-adding.
2. **No em dashes**, this repo's documented style rule (`AGENTS.md`:
   "not in docs, not in commit messages, not in code comments"). Scoped to
   lines your commit actually *adds*, not the whole staged file: this repo
   has some pre-existing em-dashes from before the rule was adopted, and a
   whole-file check would block any unrelated commit that merely touches
   one of those files. Not auto-fixed (picking a comma vs. a colon vs. a
   period needs a human), the commit is blocked with the offending
   `file:line`.
3. **`cppcheck`** on staged `.c` files (`--enable=warning,performance,
   portability`), a real static-analysis gate (uninitialized variables,
   null derefs, etc.), not a style check. `unusedFunction` and
   `missingInclude`/`missingIncludeSystem` are suppressed: cppcheck
   analyzes one file at a time here, so it cannot see that a module's
   public functions are called from its own tests/examples or from other
   modules, and would otherwise flag every public API function in every
   module as dead code. Skipped with a notice if `cppcheck` isn't
   installed.

### What it deliberately does not check yet: `clang-format`

A starter `.clang-format` is in the repo root, but it is **not** wired
into the hook. Tested against a real file (`pic8-pid/src/pid.c`) before
deciding this: even a style hand-picked to match this codebase's actual
conventions (4-space indent, `Stroustrup` brace style: own-line brace for
functions, attached brace for `if`/`while`/`else`) still reformatted
things this codebase does deliberately, aligned `struct` field/assignment
comments into columns, single-line `if (x) { y; }` clamp idioms expanded
across multiple lines, `} else {` split onto two lines. That is real
diff noise on files nobody actually changed the meaning of, not a bug in
the config, clang-format does not have a "match this file's existing
hand-tuned alignment" mode.

If you want to use it anyway for a specific file, `git clang-format
--diff` shows what would change without applying it, and only considers
lines your commit touches, not the whole file. Tightening `.clang-format`
enough to stop fighting this codebase's style (or deciding to reformat
the codebase once and live with the new style going forward) is future
work, not done here.

### Manual run

```sh
git add <files>
bash scripts/pre-commit-checks.sh
```

Runs the exact same checks the hook runs, without committing.

### CI

`.github/workflows/host-tests.yml`'s `lint` job runs this same script
against a fresh checkout, which has nothing staged (everything is already
committed). Setting `PRE_COMMIT_BASE_REF=<ref>` switches every check from
"staged index vs. `HEAD`" to "`<ref>` vs. `HEAD`", so a PR gets exactly the
same em-dash/whitespace/cppcheck rules applied to the lines it actually
changed, not the whole tree (which would also flag this repo's
pre-existing violations from before the rules were adopted). Local,
hook-driven runs are unaffected: the variable is unset there, so behavior
is identical to before.

## CI change-scoping (docs-only skip, affected-module narrowing)

Two more scripts, each with a full header comment covering the "why",
used by all three workflows to avoid paying for a docs-only PR or a PR
that only touches one module:

- `ci-docs-only-check.sh <base-ref>`: prints `true`/`false`, used by
  `xc8-build.yml` and `sim-tests.yml` to skip their Docker-based jobs
  (image pull already excepted, a `docker pull` against a cached tag is
  cheap either way) entirely on a documentation-only PR diff.
- `ci-discover-affected-modules.py [base-ref]`: used by
  `host-tests.yml`'s `discover` job. Same docs-only concept as above,
  plus a second question host-tests.yml specifically needs: which
  modules were actually touched, directly or through a sibling
  dependency (read straight from each module's own `CMakeLists.txt`,
  no separately-maintained dependency graph to drift). Conservative on
  both axes, falls back to the full, unfiltered module list the moment
  it sees a changed file it can't attribute to a known module or
  `pic8-common/`. Only ever applied to `pull_request` runs; every push
  to `master` always gets the full matrix regardless of what changed,
  so a wrong narrowing on some PR can only delay when a break is
  caught, never let it merge unverified.
