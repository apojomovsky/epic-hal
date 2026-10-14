#!/usr/bin/env python3
"""Allman brace-style gate on added lines (see scripts/README.md).

The repo brace style is Allman/Microsoft: an opening block brace owns
its line. The classifier is a small state machine over masked source:
strings, chars, comments and preprocessor spans are blanked or skipped,
and paren groups are tracked across lines within a file, so multi-line
conditions and signatures ending in `) {` are still flagged while C
compound literals ((type){ ... }), initializers, macro `do {` idioms
and type declarations (typedef/struct/union/enum {) stay exempt.
`else if` and bare `else`/`do` keep their keyword on the statement
line; only their brace must move to its own line.

Scope: lines the diff ADDS (base...HEAD via the first CLI arg, or the
staged index), so pre-existing style never blocks an unrelated commit.
Exits 1 with file:line hits.
"""

import re
import subprocess
import sys

_CTRL_KWS = ("if", "for", "while", "switch", "do")

_FNPTR_RE = re.compile(r"\(\s*\*\s*[A-Za-z_][A-Za-z0-9_]*\s*\)\s*\([^()]*$")

_TYPE_RE = re.compile(
    r"(?:^|[^A-Za-z0-9_])(?:static|const|volatile|signed|unsigned|long|short"
    r"|char|int|float|double|void|bool|uint(?:8|16|32)_t|int(?:8|16|32)_t"
    r"|(?:struct|union|enum)\s+[A-Za-z_][A-Za-z0-9_]*)$"
)


def mask_line(line, in_comment):
    """Blank strings, chars and comments in `line`, preserving layout.

    Returns (masked, in_comment_out). Block comments thread across lines.
    """
    out = list(line)
    i = 0
    n = len(line)
    while i < n:
        if in_comment:
            if line[i] == "*" and i + 1 < n and line[i + 1] == "/":
                out[i] = out[i + 1] = " "
                in_comment = False
                i += 2
            else:
                out[i] = " "
                i += 1
            continue
        c = line[i]
        if c == "/" and i + 1 < n and line[i + 1] == "/":
            for j in range(i, n):
                out[j] = " "
            break
        if c == "/" and i + 1 < n and line[i + 1] == "*":
            out[i] = out[i + 1] = " "
            in_comment = True
            i += 2
            continue
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
    return "".join(out), in_comment


def _head_is_block(head):
    """Classify the paren group `head` (text through its opening `(`):
    control keyword, function definition (incl. pointer and function-
    pointer returns), or compound-literal/expression context."""
    if _FNPTR_RE.search(head):
        return True
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
    tail = re.sub(r"[\s*]+$", "", prefix)
    if _TYPE_RE.search(tail):
        # `int f(`, `struct S *make_s(`: function definition.
        return True
    return False


def scan_file(lines, state):
    """Scan one hunk's lines for attached block braces, threading
    paren-group/comment/macro state in and out of `state` so a paren
    group opened in an earlier hunk of the same file still classifies a
    later closing line.

    `lines` is a list of (is_added, text). Returns [(index, text)] hits,
    index 0-based into `lines`."""
    hits = []
    groups = state["groups"]
    in_comment = state["in_comment"]
    pp_cont = state["pp_cont"]
    for idx, (added, raw) in enumerate(lines):
        if pp_cont:
            pp_cont = raw.rstrip().endswith("\\")
            continue
        if raw.lstrip().startswith("#"):
            pp_cont = raw.rstrip().endswith("\\")
            groups = []
            continue
        masked, in_comment = mask_line(raw, in_comment)
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
                block = False
                if closed_head is not None and pre.endswith(")"):
                    block = _head_is_block(closed_head)
                elif pre.endswith(("else", "do")):
                    block = True
                elif pre.endswith(")") and added:
                    # Unknown origin: this `)` has no opener visible in
                    # the scanned region (closed before the hunk began,
                    # e.g. an untouched `if (a &&` above an added
                    # `    b) {`). Fail closed: an attached opener after
                    # an unclassifiable close is a violation. Cross-line
                    # compound literals would false-positive here, but
                    # the repo has none; control/function position is
                    # the overwhelmingly common case.
                    block = True
                if block and added:
                    hits.append((idx, raw.strip()))
                closed_head = None
            i += 1
    state["groups"] = groups
    state["in_comment"] = in_comment
    state["pp_cont"] = pp_cont
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
    # Single walk: per-file state (paren groups, block comments,
    # preprocessor spans) threads across hunks; hits are reported only
    # for added lines with true new-file line numbers.
    hits2 = []
    state = {"groups": [], "in_comment": False, "pp_cont": False,
             "filename": None, "cur": 0, "hunk": []}
    for raw in diff.splitlines():
        if raw.startswith("+++ "):
            _flush(state, hits2)
            state["filename"] = raw[4:]
            state["groups"] = []
            state["in_comment"] = False
            state["pp_cont"] = False
            continue
        if raw.startswith("--- ") or raw.startswith("diff --git "):
            continue
        if raw.startswith("@@"):
            _flush(state, hits2)
            m = re.search(r" \+(\d+)", raw)
            state["cur"] = int(m.group(1)) if m else 1
            state["hunk"] = []
            continue
        if state["filename"] is None:
            continue
        if raw.startswith("+"):
            state["hunk"].append((True, raw[1:], state["cur"]))
            state["cur"] += 1
        elif raw.startswith(" "):
            state["hunk"].append((False, raw[1:], state["cur"]))
            state["cur"] += 1
        # removed lines do not exist in the new file: drop them
    _flush(state, hits2)
    return hits2


def _flush(state, out):
    if not state["hunk"]:
        return
    for idx, text in scan_file(
        [(a, t) for a, t, _ in state["hunk"]], state
    ):
        _, _, lineno = state["hunk"][idx]
        out.append(f"{state['filename']}:{lineno}: {text}")
    state["hunk"] = []


if __name__ == "__main__":
    hits = main()
    if hits:
        print("brace-style: opening brace must be on its own line (Allman)"
              " in added lines:")
        for h in hits:
            print("  " + h)
        sys.exit(1)
    sys.exit(0)
