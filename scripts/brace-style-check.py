#!/usr/bin/env python3
"""Allman brace-style gate on added lines (see scripts/README.md).

The repo brace style is Allman/Microsoft: an opening block brace owns its
line. Attached openers are violations:

    void f(void) {            <- violation
    if (x) {                  <- violation
    } else {                  <- violation
    do {                      <- violation (real do/while statement)

Scope: lines the diff ADDS (base...HEAD via the first CLI arg, or the
staged index), so pre-existing style never blocks an unrelated commit.
Exempt: type declarations (typedef/struct/union/enum {), initializers
(= {, array {), C compound literals ((type){), the macro `do {` idiom
(preprocessor lines), strings, chars and comments. `else if` and `do`
keep the control keyword on the statement line; only their brace must
move to its own line. A small state machine tracks paren groups across
lines within a hunk, so multi-line conditions and function signatures
ending in `) {` are still flagged while compound literals are not.
Exits 1 with file:line hits.
"""

import re
import subprocess
import sys

_CTRL_KWS = ("if", "for", "while", "switch", "do")

_TYPE_RE = re.compile(
    r"(?:^|[^A-Za-z0-9_])(?:static|const|volatile|signed|unsigned|long|short"
    r"|char|int|float|double|void|bool|uint(?:8|16|32)_t|int(?:8|16|32)_t"
    r"|(?:struct|union|enum)\s+[A-Za-z_][A-Za-z0-9_]*)$"
)


def mask_line(line):
    """Blank strings, chars and comments in `line`, preserving layout."""
    out = list(line)
    i = 0
    n = len(line)
    while i < n:
        c = line[i]
        if c == "/" and i + 1 < n and line[i + 1] in ("/", "*"):
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


def _head_is_block(head):
    """Classify the paren group whose head text (through the opening `(`)
    is `head`: control keyword, function definition, or compound literal.
    True means a `{` immediately after the group's closing `)` opens a
    real block."""
    m = re.search(r"([A-Za-z_][A-Za-z0-9_]*)\s*\($", head)
    if not m:
        # No identifier before `(`: `= (`, `, (`, `& (`, `( (` etc.
        # A cast/compound literal, not a block opener.
        return False
    word = m.group(1)
    if word in _CTRL_KWS:
        return True
    prefix = head[: m.start()].rstrip()
    if not prefix:
        # `foo(` at statement head: a function definition.
        return True
    if _TYPE_RE.search(prefix):
        # `int f(` / `struct S make_s(`: function definition.
        return True
    return False


def scan_hunk(lines):
    """Scan one contiguous region of new-file lines (context + additions,
    in order) for attached block braces on ADDED lines.

    `lines` is a list of (is_added, text). Returns a list of (0-based
    index into `lines`, stripped text) hits. Paren groups are tracked
    across lines within the hunk; a macro line resets the group stack."""
    hits = []
    groups = []
    for idx, (added, raw) in enumerate(lines):
        if raw.lstrip().startswith("#"):
            groups = []
            continue
        masked = mask_line(raw)
        closed_head = None
        i = 0
        n = len(masked)
        while i < n:
            c = masked[i]
            if c == "(":
                groups.append(raw[: i + 1])
            elif c == ")":
                if groups:
                    closed_head = groups.pop()
            elif c == "{" and not groups:
                pre = masked[:i].rstrip()
                if closed_head is not None and pre.endswith(")"):
                    block = _head_is_block(closed_head)
                else:
                    block = pre.endswith(("else", "do")) or pre == "}"
                if block and added:
                    hits.append((idx, raw.strip()))
                closed_head = None
            i += 1
        if not masked.rstrip().endswith(")"):
            closed_head = None
    return hits


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
    hunk_re = re.compile(r"^@@ -\d+(?:,\d+)? \+\d+(?:,\d+)? @@.*$", re.M)
    starts = [m.start() for m in hunk_re.finditer(diff)]
    ends = starts[1:] + [len(diff)]
    # Assign hunks to files: walk the diff marking the +++ line each
    # hunk falls under.
    hunk_files = []
    filename = None
    pos = 0
    for raw in diff.splitlines():
        hunk_match = hunk_re.search(raw)
        if raw.startswith("+++ "):
            filename = raw[4:]
        if hunk_match:
            hunk_files.append(filename)
        pos += len(raw) + 1
    for (s, e), fname in zip(zip(starts, ends), hunk_files):
        body = diff[s:e]
        m = re.search(r" \+(\d+)(?:,\d+)?", body.splitlines()[0])
        start = int(m.group(1))
        lines = []
        for ln in body.splitlines()[1:]:
            if ln.startswith("+++ ") or ln.startswith("--- "):
                break
            if ln.startswith("+"):
                lines.append((True, ln[1:]))
            elif ln.startswith(" "):
                lines.append((False, ln[1:]))
            elif ln.startswith("-"):
                continue
        for idx, text in scan_hunk(lines):
            # `start` is the 1-based new-file line of the hunk's first
            # line; idx is 0-based within the hunk.
            line_no = start + idx
            hits.append(f"{fname}:{line_no}: {text}")
    if hits:
        print("brace-style: opening brace must be on its own line (Allman)"
              " in added lines:")
        for h in hits:
            print("  " + h)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
