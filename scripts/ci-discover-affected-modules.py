#!/usr/bin/env python3
"""Decide which CMake modules host-tests.yml actually needs to build+test.

Two independent questions, both answered here so host-tests.yml's `discover`
job stays a single step:

  1. Is this change docs-only? If every changed file is a .md or lives under
     a docs/ directory, the build-test matrix is empty and the whole
     build-test job is skipped (a real cmake configure+build+ctest run
     cannot be affected by prose, and this repo has no doc-generation step
     that consumes source to produce docs, so there is no direction in
     which a docs change could break a build).
  2. If not docs-only, which modules were actually touched, directly or
     transitively? Every module's own CMakeLists.txt already declares its
     sibling dependencies via a `<NAME>_DIR ... ../<module>` pattern
     (HAL_DIR, SERIAL_DIR, TICK_DIR, TASKMGR_DIR, MATH_DIR); this script
     reads that straight from the tracked CMakeLists.txt files instead of
     hand-maintaining a graph that would drift the moment a module's own
     dependency changes. `pic8-common/` is a third, implicit dependency of
     all three HALs (included via `include()`, not `add_subdirectory()`, so
     it never appears as its own module and never appears in the graph
     above): a change under `pic8-common/` is treated as touching every HAL
     directly, same as if all three HAL directories had changed.

Conservative by construction, on both axes:
  - Any changed file that is not (a) a .md file, (b) inside a discovered
    module's own directory, or (c) inside pic8-common/ falls back to "not
    docs-only, every module affected" — a Makefile, workflow, script, or
    Docker change is exactly the kind of change this script cannot reason
    about safely, so it doesn't try.
  - The graph walk only ever grows the affected set (transitive closure),
    never prunes based on which specific files inside a dependency changed.
  - This filtering is meant for pull_request runs only (see host-tests.yml):
    every push to master still runs the unfiltered matrix, so a wrong
    closure on a PR can only ever cause an extra CI run later, never let a
    real break merge unverified.

Prints one JSON object to stdout: {"docs_only": bool, "modules": [...]}.
"""

import json
import re
import subprocess
import sys


def git(*args):
    return subprocess.run(
        ["git", *args], capture_output=True, text=True, check=True
    ).stdout


def resolve_base_ref():
    """Mirror host-tests.yml's lint-job base-ref resolution exactly, so a
    docs-only decision and the pre-commit em-dash/whitespace diff scan
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
    pulling in a sibling (see pic8-tick/CMakeLists.txt's HAL_DIR for the
    canonical example)."""
    # Matches "../<module>" followed by a word boundary: either end of
    # line (the multi-line `set(HAL_DIR ... ../pic8-tick` form, whose
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
        # docs-only rather than guessing, the safest "nothing to verify"
        # signal available.
        print(json.dumps({"docs_only": True, "modules": []}))
        return

    def is_docs(path):
        return path.endswith(".md") or "/docs/" in path or path.startswith("docs/")

    if all(is_docs(p) for p in changed):
        print(json.dumps({"docs_only": True, "modules": []}), file=sys.stdout)
        print(f"docs-only change ({len(changed)} file(s)), skipping build-test", file=sys.stderr)
        return

    modules = discover_modules()
    module_set = set(modules)

    def owning_module(path):
        # Longest matching module-dir prefix, so e.g. pic8-tick/mcu/...
        # attributes correctly even though pic8-tick contains subdirs.
        best = None
        for m in module_set:
            if path == m or path.startswith(m + "/"):
                if best is None or len(m) > len(best):
                    best = m
        return best

    touched_modules = set()
    fallback_reason = None
    for p in changed:
        if is_docs(p):
            continue
        if p.startswith("pic8-common/"):
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
        print(json.dumps({"docs_only": False, "modules": modules}))
        print(
            f"'{fallback_reason}' is outside any known module and outside "
            f"pic8-common/, falling back to the full {len(modules)}-module matrix",
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

    print(json.dumps({"docs_only": False, "modules": affected_sorted}))
    print(
        f"{len(changed)} file(s) changed, {len(touched_modules)} module(s) "
        f"directly touched, {len(affected_sorted)} affected after dependency closure: "
        f"{', '.join(affected_sorted)}",
        file=sys.stderr,
    )


if __name__ == "__main__":
    main()
