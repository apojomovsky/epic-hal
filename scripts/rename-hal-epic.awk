# HAL_ -> EPIC_ token rename, one gsub per line, guarded by an
# exceptions file (scripts/hal-epic-exceptions.txt) so genuinely
# ambiguous/foreign tokens (STM32Cube's own real API, quoted here only
# as a comparison; see docs/hal-epic-rename-plan.md) survive untouched.
# Not meant to be run directly: scripts/rename-hal-epic.sh invokes this
# once per file, with FILENAME set to that file's git-relative path (so
# it matches the exceptions file's path column) and -v EXC=<path> to the
# exceptions file itself.
#
# Protection trick: before the blind gsub, every exception token that
# applies to this file+line is swapped for a placeholder that cannot
# itself contain "HAL_" (so the blind gsub can't touch it), then swapped
# back after. All exception tokens here are plain alnum+underscore, no
# regex metacharacters, so using them directly as gsub patterns is safe
# without escaping.

BEGIN {
    while ((getline eline < EXC) > 0) {
        if (eline ~ /^#/ || eline == "") continue
        split(eline, f, "\t")
        if (f[1] == "*" && f[2] == "*") {
            wildcard[f[3]] = 1
        } else {
            key = f[1] SUBSEP f[2]
            lineExceptions[key] = f[3]
        }
    }
    close(EXC)
}

{
    line = $0
    n = 0

    for (tok in wildcard) {
        if (index(line, tok) > 0) {
            ph = sprintf("@@RENAME_PROTECT_%d@@", n)
            gsub(tok, ph, line)
            protectedText[n] = tok
            placeholder[n] = ph
            n++
        }
    }

    key = FILENAME SUBSEP FNR
    if (key in lineExceptions) {
        tok = lineExceptions[key]
        if (index(line, tok) > 0) {
            ph = sprintf("@@RENAME_PROTECT_%d@@", n)
            gsub(tok, ph, line)
            protectedText[n] = tok
            placeholder[n] = ph
            n++
        }
    }

    gsub(/HAL_/, "EPIC_", line)

    for (i = 0; i < n; i++) {
        gsub(placeholder[i], protectedText[i], line)
    }

    print line
}
