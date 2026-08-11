#!/usr/bin/env python3
"""Doxygen-style docstring compliance checker for first-party C sources.

Scans C source and header files for function declarations/definitions and
verifies that each one carries a `/** ... */` doc block containing a
`@brief`, a matching `@param` for every named parameter, no bracketed
direction tags ([in]/[out]/[in,out]), and a `@return` iff the function
returns something other than exactly `void`.

Signature parsing tolerates multi-line signatures, nested parens
(function-pointer parameters), variadic `...`, `static`/`inline`/
`const`/`volatile` qualifiers, XC8 `__interrupt` ISRs, and XC8 `__at(addr)`
attributes (also via the EPIC_PLACE(addr) macro and `__attribute__((...))`).
Anything that looks like a function but cannot be parsed is reported as
UNPARSEABLE and fails the run (fail-closed), so a human looks.

Usage:
    python3 scripts/doxygen_doc_check.py [--brief-only] FILE...

Exit status: 0 if every function is compliant, 1 otherwise (violations are
printed to stderr, one line per violation).

Violation kinds:
    missing-doc        no comment block precedes the signature
    wrong-doc-style    preceding comment is not `/** ... */`
    missing-brief      doc block has no @brief
    missing-param      @param <name> missing for a named parameter
    extra-param        @param for something that is not a named parameter
    param-direction    [in]/[out]/[in,out] tag present in the doc block
    missing-return     non-void function lacks @return
    unexpected-return  void function documents @return
    unparseable        signature-like construct the parser could not handle
"""

import argparse
import bisect
import re
import sys

# --- tokenizer helpers ------------------------------------------------------

_NAME_RE = re.compile(r"[A-Za-z_]\w*")
_ALL_CAPS_RE = re.compile(r"[A-Z][A-Z0-9_]*")
_ATTR_NAMES = ("__at", "__interrupt", "__attribute__")

# Identifiers that, when they immediately precede `name(`, mean the construct
# is control flow or an expression use rather than a function declaration.
_NON_TYPE_KEYWORDS = frozenset(
    "if while for switch case default do else goto return break continue "
    "sizeof _Alignof _Alignas _Static_assert _Generic typedef struct union "
    "enum asm __asm __asm__ extern register auto static inline const "
    "volatile"
    .split()
)

# C type keywords that can appear as the sole token of an unnamed parameter
# (and therefore are never themselves a parameter name).
_TYPE_KEYWORDS = frozenset(
    "void char short int long float double signed unsigned bool _Bool "
    "size_t ssize_t ptrdiff_t wchar_t uint8_t uint16_t uint32_t uint64_t "
    "int8_t int16_t int32_t int64_t uintptr_t intptr_t const volatile"
    .split()
)

_RETURN_QUAL_RE = re.compile(
    r"\b(?:static|inline|extern|const|volatile|register|restrict)\b")
_FUNC_PTR_NAME_RE = re.compile(r"\(\s*\*+\s*(\w+)")
_ARRAY_SUFFIX_RE = re.compile(r"\[[^\]]*\]")
_WORD_RE = re.compile(r"\b[A-Za-z_]\w*\b")
_BRIEF_RE = re.compile(r"@brief\b")
_RETURN_RE = re.compile(r"@return\b")
_PARAM_DIR_RE = re.compile(r"\[(?:in|out)(?:\s*,\s*(?:in|out))*\]")
_PARAM_TAG_RE = re.compile(r"@param\s+(\w+)")


def _is_ident_start(c):
    return ("a" <= c <= "z") or ("A" <= c <= "Z") or c == "_"


def _is_ident_char(c):
    return _is_ident_start(c) or ("0" <= c <= "9")


def _tail_terminator(masked, start):
    """Validate the text after a signature's closing paren: whitespace and
    attribute words (plus any `NAME(...)` groups, e.g. a same-signature
    variant in the other arm of a `#if/#else`), then `;` or `{`.  Returns
    the index of the `;`/`{` or None.  Linear scan (no regex backtracking).
    """
    n = len(masked)
    k = start
    while k < n and masked[k] in " \t\r\n":
        k += 1
    while k < n:
        if not _is_ident_char(masked[k]):
            break
        while k < n and _is_ident_char(masked[k]):
            k += 1
        while k < n and masked[k] in " \t\r\n":
            k += 1
        while k < n and masked[k] == "(":
            close = _match_paren(masked, k)
            if close is None:
                return None
            k = close
            while k < n and masked[k] in " \t\r\n":
                k += 1
    if k < n and masked[k] in ";{":
        return k
    return None


def _blank(masked, start, end):
    for k in range(start, end):
        if masked[k] != "\n":
            masked[k] = " "


def _skip_string(text, i):
    n = len(text)
    i += 1
    while i < n:
        if text[i] == "\\":
            i += 2
        elif text[i] == '"':
            return i + 1
        else:
            i += 1
    return n


def _skip_char(text, i):
    n = len(text)
    i += 1
    while i < n:
        if text[i] == "\\":
            i += 2
        elif text[i] == "'":
            return i + 1
        else:
            i += 1
    return n


def _match_paren(text, open_idx):
    """Index just past the `)` matching text[open_idx], or None."""
    depth = 0
    i = open_idx
    n = len(text)
    while i < n:
        c = text[i]
        if c == '"':
            i = _skip_string(text, i)
            continue
        if c == "'":
            i = _skip_char(text, i)
            continue
        if c == "(":
            depth += 1
        elif c == ")":
            depth -= 1
            if depth == 0:
                return i + 1
        i += 1
    return None


def _mask_source(text):
    """Return (masked, comments).

    masked is ``text`` with every comment, string/char literal, preprocessor
    line (with backslash continuations), XC8 attribute form (`__at(...)`,
    `__interrupt(...)`, `__attribute__(...)`) and all-caps macro invocation
    (`NAME(...)`) replaced by spaces; newlines are preserved so offsets and
    line numbers stay valid.  comments is a list of
    ``(start, end, content, kind)`` where kind is 'doc' (/** */), 'block'
    (/* */) or 'line' (//).
    """
    n = len(text)
    masked = list(text)
    comments = []
    i = 0
    while i < n:
        c = text[i]
        if c == "/" and i + 1 < n and text[i + 1] == "*":
            j = text.find("*/", i + 2)
            end = j + 2 if j != -1 else n
            kind = "doc" if i + 2 < n and text[i + 2] == "*" else "block"
            content = text[i + 3:end - 2] if kind == "doc" else ""
            comments.append((i, end, content, kind))
            _blank(masked, i, end)
            i = end
        elif c == "/" and i + 1 < n and text[i + 1] == "/":
            j = text.find("\n", i + 2)
            end = j if j != -1 else n
            comments.append((i, end, "", "line"))
            _blank(masked, i, end)
            i = end
        elif c == '"':
            end = _skip_string(text, i)
            _blank(masked, i, end)
            i = end
        elif c == "'":
            end = _skip_char(text, i)
            _blank(masked, i, end)
            i = end
        elif c == "#":
            ls = text.rfind("\n", 0, i) + 1
            if text[ls:i].strip() == "":
                j = i
                while j < n:
                    nl = text.find("\n", j)
                    if nl == -1:
                        j = n
                        break
                    if nl > j and text[nl - 1] == "\\":
                        j = nl + 1
                        continue
                    j = nl
                    break
                _blank(masked, i, j)
                i = j
            else:
                i += 1
        elif c.isalpha() or c == "_":
            m = _NAME_RE.match(text, i)
            name = m.group()
            if name in _ATTR_NAMES or _ALL_CAPS_RE.fullmatch(name):
                k = m.end()
                while k < n and text[k] in " \t":
                    k += 1
                if k < n and text[k] == "(":
                    close = _match_paren(text, k)
                    if close is not None:
                        _blank(masked, i, close)
                        i = close
                        continue
            i = m.end()
        else:
            i += 1
    return "".join(masked), comments


def _split_params(param_text):
    """Split a parameter-list string on top-level commas."""
    params = []
    depth = 0
    cur = []
    for ch in param_text:
        if ch == "(":
            depth += 1
        elif ch == ")":
            depth -= 1
        if ch == "," and depth == 0:
            params.append("".join(cur).strip())
            cur = []
        else:
            cur.append(ch)
    if cur:
        params.append("".join(cur).strip())
    return [p for p in params if p]


def _param_name(p):
    """Return the name of a single parameter declaration, or None if it has
    no name (`void`, `...`, `const char *`, `struct foo *`, `void (*)(int)`).
    """
    if p == "void" or p == "...":
        return None
    if "(" in p:
        # Function-pointer parameter: the name follows the `(*`.
        m = _FUNC_PTR_NAME_RE.search(p)
        if m:
            return m.group(1)
        # Function-typed parameter (`int mapper(int x)`): the name sits
        # before the first `(`.
        p = p[:p.find("(")]
    words = _WORD_RE.findall(_ARRAY_SUFFIX_RE.sub(" ", p))
    if not words:
        return None
    if len(words) >= 2 and words[-2] in ("struct", "union", "enum"):
        return None  # last word is the struct/union/enum tag, not a name
    last = words[-1]
    if last in _TYPE_KEYWORDS:
        return None
    return last


def _is_void_return(ret_text):
    """True when the return type is exactly `void` (after qualifiers);
    `void *` and `const void *` are therefore non-void."""
    t = _RETURN_QUAL_RE.sub(" ", ret_text)
    return re.sub(r"\s+", " ", t).strip() == "void"


def _line_starts(text):
    starts = [0]
    pos = text.find("\n")
    while pos != -1:
        starts.append(pos + 1)
        pos = text.find("\n", pos + 1)
    return starts


_PP_CONDITIONAL_RE = re.compile(
    r"^\s*#\s*(?:if|ifdef|ifndef|elif|else|endif)\b", re.MULTILINE)


def _gap_is_whitespace_or_conditional(text, start, end):
    """True when text[start:end] is only whitespace, or whitespace plus
    conditional preprocessor directives (#if/#ifdef/#ifndef/#elif/#else/
    #endif).  A doc block may associate through such a guarded-declaration
    gap, but not through #include/#define setup (a file-level doc)."""
    gap = text[start:end]
    if gap.strip() == "":
        return True
    for line in gap.splitlines():
        s = line.strip()
        if s == "":
            continue
        if not _PP_CONDITIONAL_RE.match(s):
            return False
    return True


# --- signature scanning -----------------------------------------------------

def _find_signatures(masked, unparseable):
    """Yield signature dicts from the masked text.  Constructs that look
    like functions but cannot be parsed are appended to ``unparseable``
    as (offset, name, detail) tuples (fail-closed)."""
    n = len(masked)
    last_sig_end = -1  # offset past the previous confirmed signature
    for m in _NAME_RE.finditer(masked):
        if m.start() < last_sig_end:
            continue
        j = m.end()
        while j < n and masked[j] in " \t\r\n":
            j += 1
        if j >= n or masked[j] != "(":
            continue
        # The token before the name (modulo whitespace/stars) must look like
        # a type, not a keyword or an operator context.
        p = m.start() - 1
        while p >= 0 and masked[p] in " \t\r\n*":
            p -= 1
        if p < 0 or not _is_ident_char(masked[p]):
            continue
        q = p
        while q >= 0 and _is_ident_char(masked[q]):
            q -= 1
        prev = masked[q + 1:p + 1]
        if not _is_ident_start(prev[0]) or prev in _NON_TYPE_KEYWORDS:
            continue
        close = _match_paren(masked, j)
        if close is None:
            unparseable.append((m.start(), m.group(), "unbalanced parameter list"))
            continue
        if _tail_terminator(masked, close) is None:
            unparseable.append(
                (m.start(), m.group(),
                 "unexpected text after parameter list: %r" % masked[close:close + 40]))
            continue
        # Backtrack over the return type to find the declaration start.
        d = m.start()
        while True:
            e = d
            while e > 0 and (masked[e - 1] in " \t\r\n" or masked[e - 1] == "*"):
                e -= 1
            if e > 0 and _is_ident_char(masked[e - 1]):
                s = e
                while s > 0 and _is_ident_char(masked[s - 1]):
                    s -= 1
                if not _is_ident_start(masked[s]):
                    break  # bare number, not a type word
                d = s
            else:
                break
        sig = {
            "name": m.group(),
            "name_start": m.start(),
            "decl_start": d,
            "close": close,
            "params": _split_params(masked[j + 1:close - 1]),
            "ret": masked[d:m.start()],
        }
        last_sig_end = close
        yield sig


# --- per-function checks ----------------------------------------------------

def _check_function(sig, doc, brief_only):
    """Return a list of (kind, detail) violations for one signature."""
    name = sig["name"]
    out = []
    if doc is None:
        out.append(("missing-doc", "no comment block precedes the signature"))
        return out
    if doc[3] != "doc":
        out.append(("wrong-doc-style", "comment before signature is not /** ... */"))
        return out
    content = doc[2]
    if not _BRIEF_RE.search(content):
        out.append(("missing-brief", "missing @brief"))
    if brief_only:
        return out
    if _PARAM_DIR_RE.search(content):
        out.append(("param-direction", "[in]/[out] direction tags are not allowed"))
    # Match @param names against a copy with [in]/[out]/[in,out] tags
    # stripped, so `@param[in] data` satisfies the `data` requirement and
    # param-direction is the only report for the bracket style.
    clean = _PARAM_DIR_RE.sub(" ", content)
    named = [n for n in (_param_name(p) for p in sig["params"]) if n]
    for pname in named:
        if not re.search(r"@param\s+" + re.escape(pname) + r"\b", clean):
            out.append(("missing-param", "missing @param %s" % pname))
    for m in _PARAM_TAG_RE.finditer(clean):
        tag = m.group(1)
        if tag not in named:
            out.append(("extra-param", "@param %s is not a named parameter" % tag))
    if _is_void_return(sig["ret"]):
        if _RETURN_RE.search(content):
            out.append(("unexpected-return", "void function documents @return"))
    else:
        if not _RETURN_RE.search(content):
            out.append(("missing-return", "non-void function lacks @return"))
    return out


def check_source(text, brief_only=False):
    """Check one C source string.

    Returns (violations, n_funcs) where each violation is
    (line, kind, name, detail).
    """
    masked, comments = _mask_source(text)
    starts = _line_starts(text)
    unparseable = []
    sigs = list(_find_signatures(masked, unparseable))
    # Merge same-name signatures separated only by masked (preprocessor/
    # whitespace) lines: the two arms of a `#if/#else` are one logical
    # function, checked and counted once.
    merged = []
    for sig in sigs:
        if (merged and sig["name"] == merged[-1]["name"]
                and masked[merged[-1]["close"]:sig["decl_start"]].strip() == ""):
            continue
        merged.append(sig)
    sigs = merged
    violations = []
    for offset, name, detail in unparseable:
        line = bisect.bisect_right(starts, offset)
        violations.append((line, "unparseable", name, detail))
    cidx = 0
    n_comments = len(comments)
    for sig in sigs:
        while cidx < n_comments and comments[cidx][1] <= sig["decl_start"]:
            cidx += 1
        doc = None
        if cidx > 0:
            cand = comments[cidx - 1]
            # A doc may associate through whitespace or through a
            # guarded-declaration gap (#if/#ifdef/#ifndef/#elif/#else/
            # #endif), so a doc above a `#if`-wrapped signature covers it.
            # File-level docs followed by #include/#define setup do not.
            if _gap_is_whitespace_or_conditional(text, cand[1], sig["decl_start"]):
                # A comment that has code before it on its own start line is
                # a trailing comment, not a doc attempt.
                line_start = text.rfind("\n", 0, cand[0]) + 1
                if text[line_start:cand[0]].strip() == "":
                    doc = cand
        line = bisect.bisect_right(starts, sig["decl_start"])
        for kind, detail in _check_function(sig, doc, brief_only):
            violations.append((line, kind, sig["name"], detail))
    return violations, len(sigs) + len(unparseable)


# --- CLI --------------------------------------------------------------------

def main(argv=None):
    parser = argparse.ArgumentParser(
        prog="doxygen_doc_check.py",
        description="Check C sources for doxygen-style docstrings.")
    parser.add_argument("--brief-only", action="store_true",
                        help="only require the doc block and @brief")
    parser.add_argument("files", nargs="+", metavar="FILE")
    args = parser.parse_args(argv)

    total = 0
    files_with_issues = 0
    funcs = 0
    for path in args.files:
        try:
            with open(path, encoding="utf-8", errors="replace") as fh:
                text = fh.read()
        except OSError as exc:
            sys.stderr.write("doxygen_doc_check: %s: %s\n" % (path, exc))
            return 2
        violations, n = check_source(text, brief_only=args.brief_only)
        funcs += n
        for line, kind, name, detail in violations:
            sys.stderr.write("%s:%d: %s: %s: %s\n"
                             % (path, line, kind, name, detail))
        if violations:
            files_with_issues += 1
            total += len(violations)
    sys.stderr.write("%d violation(s) in %d file(s); %d function(s) checked\n"
                     % (total, files_with_issues, funcs))
    return 1 if total else 0


if __name__ == "__main__":
    sys.exit(main())
