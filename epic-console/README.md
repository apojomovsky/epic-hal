# epic-console, line-based command dispatcher over epic-serial

Turn the existing interrupt-driven UART into a tiny shell: buffer one line,
tokenize it in place, and dispatch it through a caller-owned command table.

- **Table-driven**: the whole command set is one `static const
  epic_console_cmd_t[]`, like `epic-fsm`.
- **No hidden commands**: `epic_console_print_help()` is just a function; wire
  it to your own `"help"` row if you want one.
- **Terminal-friendly editing**: echoes typed bytes, supports backspace/DEL,
  and treats `\r`, `\n`, and `\r\n` correctly as line endings.

## Quick start

```sh
cmake -B build && cmake --build build
ctest --test-dir build --output-on-failure
./build/test_console
```

## Use it

```c
static const epic_console_cmd_t cmds[] = {
    { "status", cmd_status, "show current state" },
    { "help",   cmd_help,   "list commands" },
};

epic_console_t con;
PIC8_CONSOLE_INIT(&con, cmds, &app_ctx);
epic_console_poll(&con);
```

## License

MIT, see the [repo LICENSE](../LICENSE).
