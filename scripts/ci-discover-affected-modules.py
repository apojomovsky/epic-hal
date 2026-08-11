#!/usr/bin/env python3
"""Decide which CMake modules ci.yml's host job must build and
test: if every changed file is on the shared non-code allowlist
(ci_noncode_check.py), the matrix is empty and the job is skipped;
otherwise the affected set is the transitive closure of the modules each
CMakeLists.txt declares via its sibling <NAME>_DIR deps, plus every HAL
for epic-common/ changes. Conservative: anything unrecognized means every
module. Prints {"non_code": bool, "modules": [...]}.
"""

import json
import re
import subprocess
import sys

import ci_noncode_check


def git(*args):
    return subprocess.run(
        ["git", *args], capture_output=True, text=True, check=True
    ).stdout


def resolve_base_ref():
    """Mirror host-tests.yml's lint-job base-ref resolution exactly, so a
    non-code decision and the pre-commit em-dash/whitespace diff scan
    always agree on what range they're looking at."""
    base = sys.argv[1] if len(sys.argv) > 1 else ""
    if not base or base == "0" * 40:
        try:
            return git("rev-parse", "HEAD~1").strip()
        except subprocess.CalledProcessError:
            # No parent commit (a repo's very first commit): diff against
            # git's canonical empty-tree hash, same fallback pre-commit-
            # checks.sh's own base-ref resolution documents needing.
            return "4b825dc642cb6eb9a060e54bf8d69288fbee4904"
    return base


def discover_modules():
    out = git("ls-files", "--", "*/CMakeLists.txt")
    return sorted(line.rsplit("/CMakeLists.txt", 1)[0] for line in out.splitlines())


def build_dep_graph(modules):
    """module -> set of sibling modules it add_subdirectory()'s, read from
    each module's own CMakeLists.txt. Pattern: a `<NAME>_DIR` CMake
    variable assigned `${CMAKE_CURRENT_SOURCE_DIR}/../<module>`, which is
    the one convention every module in this repo already follows for
    pulling in a sibling (see epic-tick/CMakeLists.txt's EPIC_DIR for the
    canonical example)."""
    # Matches "../<module>" followed by a word boundary: either end of
    # line (the multi-line `set(EPIC_DIR ... ../epic-tick` form, whose
    # closing `CACHE PATH ...)` wraps to the next line) or a non-path
    # character (the single-line `... ../pic18fxx5x-hal CACHE PATH "")`
    # form). Module names in this repo are lowercase/digits/hyphen only,
    # so `[^A-Za-z0-9_-]` or end-of-string both correctly terminate the
    # match without slicing a longer name short.
    pat = re.compile(r"\.\./([A-Za-z0-9_-]+)(?:$|[^A-Za-z0-9_-])")
    graph = {m: set() for m in modules}
    module_set = set(modules)
    for m in modules:
        try:
            text = open(f"{m}/CMakeLists.txt", encoding="utf-8").read()
        except OSError:
            continue
        for line in text.splitlines():
            if "CMAKE_CURRENT_SOURCE_DIR" not in line or "_DIR" not in line:
                continue
            match = pat.search(line.rstrip())
            if match and match.group(1) in module_set and match.group(1) != m:
                graph[m].add(match.group(1))
    return graph


def transitive_closure(seeds, graph):
    seen = set(seeds)
    stack = list(seeds)
    while stack:
        m = stack.pop()
        for dep in graph.get(m, ()):
            if dep not in seen:
                seen.add(dep)
                stack.append(dep)
    return seen


def main():
    base = resolve_base_ref()
    changed = [line for line in git("diff", "--name-only", f"{base}...HEAD").splitlines() if line]

    if not changed:
        # Nothing changed in range (e.g. an empty/merge commit): treat as
        # non-code rather than guessing, the safest "nothing to verify"
        # signal available.
        print(json.dumps({"non_code": True, "modules": []}))
        return

    if ci_noncode_check.is_non_code(changed):
        print(json.dumps({"non_code": True, "modules": []}), file=sys.stdout)
        print(f"non-code change ({len(changed)} file(s)), skipping build-test", file=sys.stderr)
        return

    modules = discover_modules()
    module_set = set(modules)

    def owning_module(path):
        # Longest matching module-dir prefix, so e.g. epic-tick/mcu/...
        # attributes correctly even though epic-tick contains subdirs.
        best = None
        for m in module_set:
            if path == m or path.startswith(m + "/"):
                if best is None or len(m) > len(best):
                    best = m
        return best

    touched_modules = set()
    fallback_reason = None
    for p in changed:
        if ci_noncode_check.is_non_code([p]):
            continue
        if p.startswith("epic-common/"):
            # Implicit dependency of every HAL (include()'d, not its own
            # module): treat as touching all three HAL directories.
            touched_modules.update(m for m in modules if m.endswith("-hal"))
            continue
        m = owning_module(p)
        if m is None:
            fallback_reason = p
            break
        touched_modules.add(m)

    if fallback_reason is not None:
        print(json.dumps({"non_code": False, "modules": modules}))
        print(
            f"'{fallback_reason}' is outside any known module and outside "
            f"epic-common/, falling back to the full {len(modules)}-module matrix",
            file=sys.stderr,
        )
        return

    graph = build_dep_graph(modules)
    # Reverse the dependency graph: we need "who depends on X", not "what
    # does X depend on", to grow from touched modules outward to everything
    # that could break because of them.
    reverse_graph = {m: set() for m in modules}
    for m, deps in graph.items():
        for d in deps:
            reverse_graph[d].add(m)

    affected = transitive_closure(touched_modules, reverse_graph)
    affected_sorted = sorted(affected)

    print(json.dumps({"non_code": False, "modules": affected_sorted}))
    print(
        f"{len(changed)} file(s) changed, {len(touched_modules)} module(s) "
        f"directly touched, {len(affected_sorted)} affected after dependency closure: "
        f"{', '.join(affected_sorted)}",
        file=sys.stderr,
    )


if __name__ == "__main__":
    main()
