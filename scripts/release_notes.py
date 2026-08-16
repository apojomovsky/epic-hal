#!/usr/bin/env python3
"""Turn the Conventional Commits between two tags into the "What changed"
section of a GitHub Release. Called by .github/workflows/release-bundles.yml,
which prepends the result to the static usage block.

Commit subjects are the only source. Nothing is read from a changelog file,
so a release never disagrees with the history it was cut from.
"""
from __future__ import annotations

import argparse
import re
import subprocess
import sys

# type -> release-notes heading. Types absent here are internal: they still
# appear, collapsed, so a release never silently drops a commit.
SECTIONS = {
    "feat": "Added",
    "fix": "Fixed",
    "perf": "Changed",
    "refactor": "Changed",
    "docs": "Documentation",
}
SECTION_ORDER = ["Added", "Fixed", "Changed", "Documentation"]

# type(scope)!: subject -- scope and the breaking "!" are both optional.
SUBJECT_RE = re.compile(
    r"^(?P<type>[a-z]+)"
    r"(?:\((?P<scope>[^)]+)\))?"
    r"(?P<breaking>!)?"
    r": (?P<subject>.+)$"
)
# Trailing " (#123)" that a squash merge appends to the subject.
PR_RE = re.compile(r"\s*\(#(?P<num>\d+)\)$")
# The Conventional Commits footer, an alternative to the subject's "!".
BREAKING_RE = re.compile(r"^BREAKING[ -]CHANGE:", re.MULTILINE)

# git log record and field separators. ASCII record/unit separators: neither
# occurs in a commit message, and unlike NUL both survive being passed to
# git as a command-line argument.
_REC = "\x1e"
_FIELD = "\x1f"


class NotesError(Exception):
    """A range whose notes cannot be generated as asked."""


def _git(*args: str) -> str:
    """Run a git command and return its stdout, stripped.

    @param args in: the git subcommand and its arguments.
    @return the command's standard output with surrounding whitespace removed.
    """
    try:
        out = subprocess.run(
            ["git", *args], capture_output=True, text=True, check=True
        ).stdout
    except subprocess.CalledProcessError as e:
        raise NotesError(f"git {' '.join(args)} failed: {e.stderr.strip()}") from e
    return out.strip()


def previous_tag(tag: str) -> str | None:
    """Find the release tag immediately preceding the given one.

    Tags are ordered by version, not by commit date, so re-releases cut from
    the same commit still resolve to the right predecessor.

    @param tag in: the tag being released.
    @return the preceding tag, or None when this is the first release.
    """
    tags = _git("tag", "--sort=-v:refname").splitlines()
    tags = [t.strip() for t in tags if t.strip()]
    if tag not in tags:
        # The tag exists as a ref but is not yet listed (or a dry run passed
        # a version that was never tagged); fall back to the newest tag.
        return tags[0] if tags else None
    idx = tags.index(tag)
    return tags[idx + 1] if idx + 1 < len(tags) else None


def commits(previous: str | None, tag: str) -> list[tuple[str, str]]:
    """Collect the commits that a release introduces.

    Bodies come along because a breaking change may be declared either by
    "!" in the subject or by a BREAKING CHANGE: footer in the body.

    @param previous in: the preceding tag, or None to take all history.
    @param tag in: the tag being released.
    @return a (subject, body) pair per commit in the range, newest first.
    """
    rev = tag if previous is None else f"{previous}..{tag}"
    out = _git("log", "--no-merges", f"--pretty=format:%s{_FIELD}%b{_REC}", rev)
    records = []
    for raw in out.split(_REC):
        raw = raw.strip("\n")
        if not raw.strip():
            continue
        subject, _, body = raw.partition(_FIELD)
        records.append((subject.strip(), body))
    return records


def classify(subject: str, body: str = "") -> dict:
    """Parse one Conventional Commit into its release-notes parts.

    A subject that does not conform is not dropped: it is returned with a
    None heading so the caller can file it under the collapsed internal
    section rather than lose it.

    @param subject in: the raw commit subject line.
    @param body in: the commit body, searched for a BREAKING CHANGE: footer.
    @return a dict with heading, scope, text, pr, and breaking keys.
    """
    footer_breaking = bool(BREAKING_RE.search(body or ""))
    pr = None
    m = PR_RE.search(subject)
    if m:
        pr = m.group("num")
        subject = subject[: m.start()]

    m = SUBJECT_RE.match(subject)
    if not m:
        return {
            "heading": None,
            "scope": None,
            "text": subject.strip(),
            "pr": pr,
            "breaking": footer_breaking,
        }

    return {
        "heading": SECTIONS.get(m.group("type")),
        "scope": m.group("scope"),
        "text": m.group("subject").strip(),
        "pr": pr,
        "breaking": bool(m.group("breaking")) or footer_breaking,
    }


def _bullet(entry: dict, repo_url: str | None) -> str:
    """Render one parsed commit as a markdown list item.

    @param entry in: a dict as returned by classify.
    @param repo_url in: the repository URL used to link a PR number, or None.
    @return the markdown bullet, without a trailing newline.
    """
    scope = f"**{entry['scope']}**: " if entry["scope"] else ""
    line = f"- {scope}{entry['text']}"
    if entry["pr"]:
        if repo_url:
            line += f" ([#{entry['pr']}]({repo_url}/pull/{entry['pr']}))"
        else:
            line += f" (#{entry['pr']})"
    return line


def render(records: list, previous: str | None, tag: str,
           repo_url: str | None = None) -> str:
    """Build the complete "What changed" markdown section.

    @param records in: the release's commits, each a subject string or a
           (subject, body) pair.
    @param previous in: the preceding tag, or None for a first release.
    @param tag in: the tag being released.
    @param repo_url in: repository URL for PR and compare links, or None.
    @return the markdown section, ending in a newline.
    """
    entries = [
        classify(r, "") if isinstance(r, str) else classify(r[0], r[1])
        for r in records
    ]
    lines = ["## What changed", ""]

    if not entries:
        # Re-releases from an unchanged tree are a real case here: two 0.3.x
        # tags were cut to republish assets, not to ship code.
        base = f" since {previous}" if previous else ""
        lines.append(f"No source changes{base}; republished for the assets below.")
        lines.append("")
        return "\n".join(lines)

    breaking = [e for e in entries if e["breaking"]]
    if breaking:
        lines.append("### Breaking")
        lines.append("")
        lines += [_bullet(e, repo_url) for e in breaking]
        lines.append("")

    for heading in SECTION_ORDER:
        chosen = [e for e in entries if e["heading"] == heading and not e["breaking"]]
        if not chosen:
            continue
        lines.append(f"### {heading}")
        lines.append("")
        lines += [_bullet(e, repo_url) for e in chosen]
        lines.append("")

    internal = [e for e in entries if e["heading"] is None and not e["breaking"]]
    if internal:
        lines.append("<details><summary>Internal changes</summary>")
        lines.append("")
        lines += [_bullet(e, repo_url) for e in internal]
        lines.append("")
        lines.append("</details>")
        lines.append("")

    if previous and repo_url:
        lines.append(f"Full changelog: {repo_url}/compare/{previous}...{tag}")
        lines.append("")

    return "\n".join(lines)


def main(argv: list[str] | None = None) -> int:
    """Print the "What changed" section for a tag to stdout.

    @param argv in: argument vector, or None to read sys.argv.
    @return the process exit status.
    """
    p = argparse.ArgumentParser(
        description="Generate a release's What-changed section from git history")
    p.add_argument("tag", help="the tag being released, e.g. v0.4.0")
    p.add_argument("--previous",
                   help="override the preceding tag (default: the prior version tag)")
    p.add_argument("--repo-url",
                   help="repository URL for PR and compare links, "
                        "e.g. https://github.com/apojomovsky/epic-hal")
    args = p.parse_args(argv)

    try:
        previous = args.previous or previous_tag(args.tag)
        records = commits(previous, args.tag)
    except NotesError as e:
        print(f"release_notes: {e}", file=sys.stderr)
        return 1

    sys.stdout.write(render(records, previous, args.tag, args.repo_url))
    return 0


if __name__ == "__main__":
    sys.exit(main())
