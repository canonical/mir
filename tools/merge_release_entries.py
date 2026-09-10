#!/usr/bin/env python3
"""Git merge driver for release-artifact files whose format is a list of dated
entries with the newest entry at the top: debian/changelog, rpm/mir.spec and
doc/sphinx/release-notes.md.

A plain `merge=union` driver concatenates the conflicting hunk from our side
followed by their side, but does so line-by-line. This driver instead parses
each side into whole entries and rebuilds the file as:

    <entries added on our side, unchanged>
    <entries added on their side, unchanged>
    <entries common to both sides, i.e. present in the merge base>

For rpm/mir.spec it also bumps the date of the top `~dev` entry to today, since
the RPM tools reject unordered dates in the changelog.

Everything before the first entry (e.g. the rpm spec's `%bcond`/`Version:`
preamble, or the release notes' title and template comment) is merged with
`git merge-file`, leaving real conflicts there as ordinary conflict markers.

Registered in .gitattributes as the `merge=release-notes` driver, invoked as:

    tools/merge_release_entries.py %O %A %B %P
"""

import datetime
import re
import subprocess
import sys
import tempfile
from pathlib import Path

# Marks the end of the preamble that precedes the first entry.
PREAMBLE_END = {
    "debian/changelog": None,
    "rpm/mir.spec": "%changelog\n",
    "doc/sphinx/release-notes.md": "-->\n",
}

ENTRY_START = {
    "debian/changelog": re.compile(r"^mir \(", re.MULTILINE),
    "rpm/mir.spec": re.compile(r"^\* ", re.MULTILINE),
    "doc/sphinx/release-notes.md": re.compile(r"^## Mir ", re.MULTILINE),
}

# e.g. "* Thu Aug 13 2026 Mir CI Bot <mir-ci-bot@canonical.com> - 2.30.0~dev-1"
SPEC_ENTRY_HEADER = re.compile(r"^\* (\w{3} \w{3} \d{1,2} \d{4}) (.*~dev.*)$")


def split_preamble(path_key, text):
    marker = PREAMBLE_END[path_key]
    if marker is None:
        return "", text
    idx = text.index(marker) + len(marker)
    entry_start = ENTRY_START[path_key].search(text, idx)
    if entry_start is None:
        return text, ""
    return text[: entry_start.start()], text[entry_start.start() :]


def split_entries(path_key, text):
    """Split entries text into a list of whole entries, in file order.

    Anything before the first matched line (e.g. a blank line separating the
    preamble from the first entry) is discarded, not treated as an entry.
    """
    pattern = ENTRY_START[path_key]
    entries = []
    current = []
    for line in text.split("\n"):
        if pattern.match(line):
            if current:
                entries.append("\n".join(current).strip("\n"))
            current = [line]
        elif current:
            current.append(line)
    if current:
        entries.append("\n".join(current).strip("\n"))
    return entries


def merge_entries(base_entries, ours_entries, theirs_entries):
    ours_new = [e for e in ours_entries if e not in base_entries]
    theirs_new = [
        e for e in theirs_entries if e not in base_entries and e not in ours_new
    ]
    common = [e for e in base_entries if e in ours_entries and e in theirs_entries]
    return ours_new + theirs_new + common


def fix_dev_date(path_key, entries):
    """Bump the date of the top `~dev` entry in rpm/mir.spec to today."""
    if path_key != "rpm/mir.spec" or not entries:
        return entries
    match = SPEC_ENTRY_HEADER.match(entries[0].split("\n", 1)[0])
    if not match:
        return entries
    today = (
        datetime.datetime.now(tz=datetime.timezone.utc).date().strftime("%a %b %d %Y")
    )
    lines = entries[0].split("\n")
    lines[0] = f"* {today} {match.group(2)}"
    return ["\n".join(lines)] + entries[1:]


def merge_preamble(base_pre, ours_pre, theirs_pre):
    """3-way merge the preamble, returning (text, had_conflict)."""
    if ours_pre == theirs_pre:
        return ours_pre, False
    if ours_pre == base_pre:
        return theirs_pre, False
    if theirs_pre == base_pre:
        return ours_pre, False

    with tempfile.TemporaryDirectory() as tmp:
        tmp = Path(tmp)
        (tmp / "base").write_text(base_pre)
        (tmp / "ours").write_text(ours_pre)
        (tmp / "theirs").write_text(theirs_pre)
        result = subprocess.run(
            ["git", "merge-file", "-p", "ours", "base", "theirs"],
            cwd=tmp,
            capture_output=True,
            text=True,
            check=False,
        )
        return result.stdout, result.returncode != 0


def merge(path_key, base_text, ours_text, theirs_text):
    base_pre, base_body = split_preamble(path_key, base_text)
    ours_pre, ours_body = split_preamble(path_key, ours_text)
    theirs_pre, theirs_body = split_preamble(path_key, theirs_text)

    preamble, conflict = merge_preamble(base_pre, ours_pre, theirs_pre)

    entries = merge_entries(
        split_entries(path_key, base_body),
        split_entries(path_key, ours_body),
        split_entries(path_key, theirs_body),
    )
    entries = fix_dev_date(path_key, entries)

    body = "\n\n".join(entries)
    if body:
        body += "\n"
    return preamble + body, conflict


def main():
    base_path, ours_path, theirs_path, repo_path = sys.argv[1:5]
    if repo_path not in ENTRY_START:
        sys.exit(f"merge_release_entries: unsupported path {repo_path!r}")

    merged, conflict = merge(
        repo_path,
        Path(base_path).read_text(),
        Path(ours_path).read_text(),
        Path(theirs_path).read_text(),
    )
    Path(ours_path).write_text(merged)
    sys.exit(1 if conflict else 0)


if __name__ == "__main__":
    main()
