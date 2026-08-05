# epic-console API

## Public header

```c
#include "epic_console.h"
```

The public header depends only on `<stdint.h>` and `<stdbool.h>`.

## Build-time knobs

### `EPIC_CONSOLE_LINE_MAX`

Maximum buffered line length including the terminating `'\0'`. Defaults to
`32u`. Once full, additional input bytes are ignored until space is freed by
backspace or the line is dispatched.

### `EPIC_CONSOLE_MAX_ARGS`

Maximum number of tokens placed in `argv[]` during dispatch. Defaults to `8u`.
Extra tokens beyond that are ignored.

## Types

### `epic_console_cmd_t`

One command-table row:

- `name`: command name to match against `argv[0]`
- `handler`: callback invoked on a match
- `help`: one-line description printed by `epic_console_print_help`

### `epic_console_t`

One console instance: command table pointer, table length, opaque context
pointer, fixed line buffer, current line length, and CR/LF state.

## Functions

### `void epic_console_init(...)`

Initializes an instance with a table, row count, and opaque caller context.

### `EPIC_CONSOLE_INIT(con, table, ctx)`

Convenience macro that computes `table_len` via `sizeof(table) /
sizeof(table[0])`. As with `FSM_INIT`, call it with the actual array, not a
decayed pointer.

### `void epic_console_poll(epic_console_t *con);`

Drain all bytes currently available from `epic-serial`, echo/edit them, and
dispatch any complete lines found during that drain.

Handlers receive the conventional C shape:

- `argc >= 1` for a non-empty matched command
- `argv[0]` is the command name itself
- `ctx` is the same opaque pointer passed at init

### `void epic_console_print_help(const epic_console_t *con);`

Prints one `name - help` line per table row through `epic-serial`. This is not
auto-registered to any command name; callers wire it to their own `"help"` row
if desired.
