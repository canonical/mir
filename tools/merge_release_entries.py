#!/usr/bin/env python3
"""Git merge driver for release-artifact files whose format is a list of dated
entries with the newest entry at the top: debian/changelog and rpm/mir.spec.
For doc/sphinx/release-notes/index.md, only release-status changes on the landing
page are reconciled; individual release pages use Git's normal merge behavior.

A plain `merge=union` driver concatenates the conflicting hunk from our side
followed by their side, but does so line-by-line. This driver instead parses
each side into whole entries and rebuilds the file as:

    <entries added on our side, unchanged>
    <entries added on their side, unchanged>
    <entries common to both sides, i.e. present in the merge base>

For rpm/mir.spec it also bumps the date of the top `~dev` entry to today, since
the RPM tools reject unordered dates in the changelog.

Everything before the first packaging entry (e.g. the rpm spec's `%bcond`/`Version:`
preamble) is merged with
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
}

ENTRY_START = {
    "debian/changelog": re.compile(r"^mir \(", re.MULTILINE),
    "rpm/mir.spec": re.compile(r"^\* ", re.MULTILINE),
}

LANDING_PAGE = "doc/sphinx/release-notes/index.md"
RELEASE_LINK = re.compile(
    r"^(?P<link>- (?:\{doc\}`Mir (?P<local_version>\d+\.\d+) <(?P=local_version)>`"
    r"|\[Mir (?P<versioned_version>\d+\.\d+)\]"
    r"\(\.\./\.\./(?P=versioned_version)/release-notes/(?P=versioned_version)/\)))"
    r"(?P<status> \((?:in development|[Rr]elease [Cc]andidate)\))?$",
    re.MULTILINE,
)

# e.g. "* Thu Aug 13 2026 Mir CI Bot <mir-ci-bot@canonical.com> - 2.30.0~dev-1"
SPEC_ENTRY_HEADER = re.compile(r"^\* (\w{3} \w{3} \d{1,2} \d{4}) (.*~dev.*)$")
SPEC_VERSION = re.compile(r"^Version:.*$", re.MULTILINE)


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
    starts = [match.start() for match in pattern.finditer(text)]
    # For each entry start, slice from that start to the next entry start (or
    # the end of the file), then strip only the trailing newline so each item is
    # a complete entry block without the leading separator whitespace.
    return [
        text[start:end].strip("\n")
        for start, end in zip(starts, [*starts[1:], len(text)])
    ]


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
    if path_key == LANDING_PAGE:
        return merge_landing_page(base_text, ours_text, theirs_text)

    base_pre, base_body = split_preamble(path_key, base_text)
    ours_pre, ours_body = split_preamble(path_key, ours_text)
    theirs_pre, theirs_body = split_preamble(path_key, theirs_text)

    if path_key == "rpm/mir.spec":
        ours_version = SPEC_VERSION.search(ours_pre)
        if ours_version:
            theirs_pre = SPEC_VERSION.sub(ours_version.group(), theirs_pre)

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


def merge_landing_page(base_text, ours_text, theirs_text):
    def links(text):
        return {
            match["local_version"] or match["versioned_version"]: match
            for match in RELEASE_LINK.finditer(text)
        }

    base_links = links(base_text)
    ours_links = links(ours_text)
    theirs_links = links(theirs_text)
    for version, base_link in base_links.items():
        ours_link = ours_links.get(version)
        theirs_link = theirs_links.get(version)
        if ours_link is None or theirs_link is None:
            continue
        if ours_link.group() == base_link.group():
            resolved = theirs_link.group()
        elif (
            theirs_link.group() == base_link.group()
            or ours_link.group() == theirs_link.group()
        ):
            resolved = ours_link.group()
        else:
            resolved = None
            for candidate, incoming in [
                (ours_link, theirs_link),
                (theirs_link, ours_link),
            ]:
                if (
                    candidate["versioned_version"]
                    and (candidate["status"] or "").lower() == " (release candidate)"
                    and incoming["local_version"]
                    and (incoming["status"] or "").lower()
                    in ("", " (release candidate)")
                ):
                    resolved = (
                        incoming.group()
                        if not incoming["status"]
                        else candidate.group()
                    )
            if resolved is None:
                continue
        base_text = base_text.replace(base_link.group(), resolved)
        ours_text = ours_text.replace(ours_link.group(), resolved)
        theirs_text = theirs_text.replace(theirs_link.group(), resolved)
    return merge_preamble(base_text, ours_text, theirs_text)


def main():
    base_path, ours_path, theirs_path, repo_path = sys.argv[1:5]
    if repo_path not in ENTRY_START and repo_path != LANDING_PAGE:
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
