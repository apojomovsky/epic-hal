# epic-console architecture

## Model

`epic-console` is a thin polling layer on top of `epic-serial`. RX bytes are
already collected by the serial module's interrupt-driven ring buffer; this
module simply drains whatever is available, echoes it back, edits a fixed-size
line buffer, and dispatches a completed line through a caller-owned command
table.

The command table is the whole command set:

```c
static const epic_console_cmd_t cmds[] = {
    { "status", cmd_status, "show current state" },
    { "help",   cmd_help,   "list commands" },
};
```

That keeps the dispatch readable at a glance and avoids hidden registration or
module-global state.

## Buffering and tokenization

- `EPIC_CONSOLE_LINE_MAX` sets the fixed line-buffer size.
- `EPIC_CONSOLE_MAX_ARGS` sets the fixed `argv[]` capacity during dispatch.
- Tokenization is in-place and whitespace-based.
- The module does not use `strtok`, so there is no hidden global tokenizer
  state and multiple console instances remain independent.

If the line buffer fills, additional characters are ignored until the user
either backspaces or terminates the line. This keeps behavior bounded and
non-crashing without allocating more storage.

## Line endings and editing

- Ordinary bytes are echoed and appended to the line buffer.
- `0x08` and `0x7F` erase one buffered character and emit `"\b \b"` so a
  normal terminal visibly erases it.
- `\r`, `\n`, and `\r\n` all terminate one logical line.
- A lone `\n` immediately after a `\r` is ignored so `\r\n` does not dispatch
  an empty second command.

Unknown command names are silently ignored. This matches the same "no policy"
stance other small core modules in this repo take: the library dispatches when
it finds a match but does not impose logging or error handling on the caller.
