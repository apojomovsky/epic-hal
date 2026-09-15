#!/usr/bin/env python3
"""Allman brace-style gate on added lines (see scripts/README.md).

The repo brace style is Allman/Microsoft: an opening block brace owns its
line. Attached openers are violations:

    void f(void) {            <- violation
    if (x) {                  <- violation
    } else {                  <- violation

Scope: lines the diff ADDS (base...HEAD via the first CLI arg, or the
staged index), so pre-existing style never blocks an unrelated commit.
Exempt: type declarations (typedef/struct/enum {), initializers (= {,
comma {, array {), C compound literals ((type){), the macro `do {`
idiom, strings, chars and comments. `else if` and `do` keep the control
keyword on the statement line; only their brace must move to its own
line. Exits 1 with file:line hits.
"""

import re
import subprocess
import sys


def mask_line(line):
    """Blank strings, chars and comments in `line`, preserving layout."""
    out = list(line)
    i = 0
    n = len(line)
    while i < n:
        c = line[i]
        if c == "/" and i + 1 < n and line[i + 1] == "/":
            for j in range(i, n):
                out[j] = " "
            break
        if c == "/" and i + 1 < n and line[i + 1] == "*":
            for j in range(i, n):
                out[j] = " "
            break
        if c in ("'", '"'):
            q = c
            j = i + 1
            while j < n:
                if line[j] == "\\":
                    j += 2
                    continue
                if line[j] == q:
                    j += 1
                    break
                j += 1
            if j > n:
                j = n
            for k in range(i, j):
                out[k] = " "
            i = j
            continue
        i += 1
    return "".join(out)


def is_attached_block_opener(masked, line):
    """True if `line` opens a block with an attached brace."""
    m = re.search(r"\{", masked)
    if not m:
        return False
    idx = m.start()
    before = masked[:idx].rstrip()
    if not before:
        return False
    original_prefix = line[:idx]
    if re.match(r"^\s*(//|/?\*)", original_prefix):
        return False
    if line.lstrip().startswith("#"):
        # Preprocessor line (macro with attached brace): not a C block.
        return False
    if re.search(r"(?:^|\s)(struct|union|enum)(\s|$)", original_prefix):
        return False
    if before.endswith("else"):
        return True
    if before == "}":
        return True
    if before.endswith(")"):
        # The preceding `)` closes either a control condition or a
        # function definition, or it closes a C compound literal
        # ((type){ ... }). Scan back to the matching `(`.
        depth = 0
        open_idx = None
        for j in range(len(before) - 1, -1, -1):
            if before[j] == ")":
                depth += 1
            elif before[j] == "(":
                depth -= 1
                if depth == 0:
                    open_idx = j
                    break
        if open_idx is None:
            return False
        head = before[:open_idx].rstrip()
        if not head:
            return False  # `(` at statement head: compound literal
        mword = re.search(r"[A-Za-z_][A-Za-z0-9_]*$", head)
        if not mword:
            # Preceded by `(`, `=`, `&`, `,`, `[`, `!`, `return`, etc:
            # a compound-literal or expression-literal, not a block.
            return False
        kw = mword.group(0)
        if kw in ("if", "for", "while", "switch", "do"):
            return True
        # Identifier before `(`: function definition when its own prefix
        # is a type (or none). A call as statement cannot be followed by
        # a block in valid C, so treat as function definition.
        tail = before[: open_idx - mword.end()].rstrip()
        if not tail:
            return True
        if re.search(r"(?:^|[^A-Za-z0-9_])(?:static|const|volatile|signed"
                     r"|unsigned|long|short|char|int|float|double|void|bool"
                     r"|uint(?:8|16|32)_t|int(?:8|16|32)_t)$", tail):
            return True
        return False
    return False


def main():
    base = sys.argv[1] if len(sys.argv) > 1 else ""
    if base:
        diff_cmd = ["git", "diff", "-U0", f"{base}...HEAD",
                    "--diff-filter=ACM", "--", "*.c", "*.h"]
    else:
        diff_cmd = ["git", "diff", "-U0", "--cached",
                    "--diff-filter=ACM", "--", "*.c", "*.h"]
    p = subprocess.run(diff_cmd, capture_output=True, text=True)
    diff = p.stdout
    hits = []
    filename = None
    newline = 0
    for raw in diff.splitlines():
        if raw.startswith("+++ "):
            filename = raw[4:]
            continue
        if raw.startswith("@@"):
            m = re.search(r"\+(\d+)(?:,(\d+))?", raw)
            newline = int(m.group(1)) if m else 0
            continue
        if raw.startswith("+") and not raw.startswith("+++"):
            content = raw[1:]
            if "{" in content:
                masked = mask_line(content)
                if is_attached_block_opener(masked, content):
                    hits.append(f"{filename}:{newline}: {content.strip()}")
            newline += 1
    if hits:
        print("brace-style: opening brace must be on its own line (Allman)"
              " in added lines:")
        for h in hits:
            print("  " + h)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
