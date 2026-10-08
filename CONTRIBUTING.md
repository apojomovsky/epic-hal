# Contributing to Epic HAL

Thanks for stopping by. Epic HAL is a fully open source (MIT), register-level
HAL for 8-bit PIC microcontrollers, plus drop-in modules for the things
firmware always needs: scheduling, serial, storage, control, and math. Whether
you fix a typo, cite a datasheet bit, or bring up a whole new device, your
help is welcome.

## What the project is

One datasheet-faithful API across every supported PIC family, with each
family implementing the same contract over its own registers. If you are new
here, start with the [README](README.md) and try the one-command quickstart
before changing anything.

## Where help is welcome

- **Bug reports and datasheet corrections.** If a register bit disagrees with
  the datasheet, that is a first-class bug: open an issue with the part, the
  register, and the datasheet section.
- **Good first issues.** Look for issues labelled `good first issue`: small,
  well-scoped tasks, often docs or single-peripheral work.
- **New devices and peripherals.** See [docs/adding-a-device.md](docs/adding-a-device.md),
  the step-by-step playbook for bringing up a device or family.
- **Demos and docs.** The [demos](demos/) always need exercising on real
  parts, and unclear docs are bugs too.

Questions belong in GitHub issues or Discussions, whichever the repository
offers. There are no silly questions about banking or config words.

## Setup

You need almost nothing to contribute docs, host-side logic, or reviews: a
clone plus a C compiler and CMake is enough for the fast inner loop.

```sh
cmake -B build && cmake --build build && ctest
```

Real-target work (XC8 cross-compiles and the `mdb` simulator gate) runs
through Docker so your machine stays clean. The full walkthrough, native and
Docker paths alike, lives in [DEVELOPMENT.md](DEVELOPMENT.md).

### The `mdb` gate is real

When CI runs firmware under MPLAB SIM, it checks actual register and UART
output, not just "it compiled". A failing `mdb` gate means the target code
is wrong, never that the check is flaky. If you hit one, debug the firmware
with the protocol in [docs/adding-a-device.md](docs/adding-a-device.md)
instead of loosening the assertion.

## Adding a driver or a device

- **A peripheral driver** follows the shared contract in
  [common/MANUAL.md](common/MANUAL.md): same names and signatures in every
  family, different bodies. Cite the datasheet section for every bit.
- **A new device or family** follows [docs/adding-a-device.md](docs/adding-a-device.md)
  end to end. It is verification-gated: host tests first, then real-target
  builds, then the `mdb` gate.
- **Run the tests** for what you touched (`ctest` for host-sim), plus the
  demos that exercise it. If CI covers a path you cannot run locally, say so
  in the pull request.

## Conventions (linked, not copied)

The repository has a short list of hard rules. They live in
[AGENTS.md](AGENTS.md) and [DEVELOPMENT.md](DEVELOPMENT.md); the summary:

- Commits follow Conventional Commits (`fix(scope): summary`), one line.
- No em-dashes in prose, comments, or commit messages.
- Feature work happens in a worktree under `.worktrees/`, never on `master`.
- Run `make pre-pr-check` before opening a pull request.
- Non-trivial work starts with a short design reviewed before implementation.

That last file, AGENTS.md, also documents the agent workflow (review gate,
takeoff ritual, board tracking). It is written for automation; this file is
the one for humans. When the two disagree about human contributions, ask.

## Where the docs live

- [README](README.md): what the project is and the fastest way to use it.
- [DEVELOPMENT.md](DEVELOPMENT.md): toolchain, builds, CI, and the pre-PR gate.
- [common/MANUAL.md](common/MANUAL.md): shared API conventions and the
  interrupt model. Per-family `MANUAL.md` files cover registers.
- [docs/adding-a-device.md](docs/adding-a-device.md): the new-device playbook.
- [docs/getting-started.md](docs/getting-started.md): install paths beyond
  the one-liner. [docs/examples.md](docs/examples.md): more API examples.

If a change touches behavior, update the docs it touches in the same pull
request.

## License

MIT, see [LICENSE](LICENSE). Datasheets stay Microchip's property and are
never vendored here; link Microchip's hosted copies instead.
