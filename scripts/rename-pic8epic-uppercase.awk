# PIC8_ -> EPIC_ uppercase rename, one line at a time. Driven by
# scripts/rename-pic8epic-uppercase.sh, which calls this once per file.
# See docs/pic8-epic-uppercase-rename-plan.md.
#
# Two rules, in order. No exceptions file: the survey found zero foreign
# PIC8_ (the only [A-Z0-9]PIC8_ substring hits are -DPIC8_ compile flags,
# renamed correctly here) and no vendored PIC8_ (m-stack has none).
#
# Rule 1: PIC8_EPIC_ -> EPIC_  (collapse the double prefix the HAL_ pass
#   left in PIC8_HAL_* -> PIC8_EPIC_*, so PIC8_EPIC_LIB -> EPIC_LIB, not
#   EPIC_EPIC_LIB). Per the user's preference; it is a unification, not a
#   collision (see the plan doc). MUST run before rule 2.
# Rule 2: PIC8_ -> EPIC_  (everything else; case-sensitive, so lowercase
#   pic8_ is untouched and PIC8_ inside PIC8_EPIC_ is already gone).

{
    line = $0
    gsub(/PIC8_EPIC_/, "EPIC_", line)
    gsub(/PIC8_/, "EPIC_", line)
    print line
}
