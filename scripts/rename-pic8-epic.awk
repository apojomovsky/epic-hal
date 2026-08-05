# pic8_ / pic8- -> epic_ / epic- content rename, one line at a time.
# Driven by scripts/rename-pic8-epic.sh, which calls this once per file
# with FILENAME = the git-relative path and -v MODS=<path> pointing at
# scripts/pic8-epic-modules.txt (the 17 module names). See
# docs/pic8-epic-rename-plan.md.
#
# Three rules, in order. Unlike the HAL_ rename (scripts/rename-hal-
# epic.awk) this needs NO exceptions file: the hyphen replace is a
# *whitelist* of the 17 module names + the pic8-* glob, so every
# non-module pic8- token (pic8-hal*, pic8-vga, pic8-ramp, pic8-family)
# is preserved by construction; and the pic8_ gsub is case-sensitive,
# so uppercase PIC8_ macros are preserved by construction.
#
# Rule 1: pic8_ -> epic_ (case-sensitive, lowercase only). Safe because
#   the survey found zero foreign lowercase pic8_ and no [a-z0-9]pic8_
#   substring hits; PIC8_ uppercase is never matched.
# Rule 2: pic8-<module> -> epic-<module> for each of the 17 module
#   names (literal string replace; no module name is a substring of a
#   longer alnum token, and module-suffix forms like pic8-usb-specific
#   or pic8-<module>-plan.md rename correctly), plus pic8-* -> epic-*
#   (the set/glob reference).
# Rule 3: github.com/apojomovsky/pic8-hal -> github.com/apojomovsky/
#   epicurus (the repo was renamed to epicurus). Scoped to the
#   github.com/ host so ghcr.io/apojomovsky/pic8-hal-ci (the GHCR
#   package, kept) is never matched.

BEGIN {
    while ((getline mline < MODS) > 0) {
        if (mline ~ /^#/ || mline == "") continue
        mods[mline] = 1
    }
    close(MODS)
}

{
    line = $0

    # Rule 1: lowercase pic8_ -> epic_ (case-sensitive).
    gsub(/pic8_/, "epic_", line)

    # Rule 2: pic8-<module> -> epic-<module> for each whitelisted module,
    # then the pic8-* glob.
    for (m in mods) {
        gsub("pic8-" m, "epic-" m, line)
    }
    gsub(/pic8-\*/, "epic-*", line)

    # Rule 3: GitHub repo URL -> epicurus (host-scoped).
    gsub(/github\.com\/apojomovsky\/pic8-hal/, "github.com/apojomovsky/epicurus", line)

    print line
}
