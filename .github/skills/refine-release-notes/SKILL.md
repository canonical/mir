---
name: refine-release-notes
description: Refine a Mir per-release documentation page and its concise Debian changelog entry using the release-notes template, the GitHub-generated release notes comment, and the history and diff since the previous release.
---

# Refine Release Notes

Use this workflow during development or after `Start Release` opens a release PR.
Refine the target point-release section in `doc/sphinx/release-notes/X.Y.md` using
`doc/sphinx/release-notes/template.include.md`, and the concise packaging entry in
`debian/changelog` using `.github/workflows/changelog-template`.
The documentation section supplies the GitHub release body; the Debian entry
supplies the RPM changelog. Edit neither generated output directly.

## Format

The documentation template follows the
[Sphinx Stack release-notes example](https://documentation.ubuntu.com/sphinx-stack/latest/release-notes/),
with these Mir-specific conventions:

- Use one `X.Y.md` page per MAJOR.MINOR series, titled `Mir X.Y`, with version-only
  `X.Y` labels in the landing-page TOC. Merge point releases into that page.
- Keep point releases newest first, delimited by invisible `(release-X.Y.Z)=`
  anchors for workflow extraction. Do not add redundant version headings,
  a "release notes" title suffix, or introductory sentences such
  as "These release notes cover fixes in...".
- Use second-level headings for `Requirements and compatibility`,
  `What's new in Mir X.Y.Z`, `Minor features`, `Backwards-incompatible changes`,
  and `Deprecated features`; use descriptive third-level headings for features.
  Keep the `in Mir X.Y.Z` suffix only in `What's new in...` headings.
- Use the full point version for a patch release's changes heading, for example
  `## [Mir 2.25.2](https://github.com/canonical/mir/releases/v2.25.2)`, instead of
  `Minor features`. Feature releases can retain `Minor features`.
- In every heading containing a full version, link the `Mir X.Y.Z` text to
  `https://github.com/canonical/mir/releases/vX.Y.Z`.
- Show only the latest date on a multi-release page, as plain text, not a heading.
  Leave `Unreleased` for the RC workflow to mark `Release Candidate`, which
  finalization replaces with the date. Remove obsolete date paragraphs while
  preparing the release section, preserving older release content and anchors.
- Preserve the page's frontmatter and minor-series anchor. Remove empty sections,
  template comments and placeholders before release. ABI summaries belong only in
  packaging, not in the documentation page.
- Group the landing page by Ubuntu series: Mir 2.29 and higher under `26.04`,
  2.18 through 2.28 under `24.04`, and 2.14 through 2.17 under `22.04`.
  Keep this existing grouping; do not recompute it during release workflows.
- Where no local notes page exists, link only the latest point release for each
  minor series on GitHub. Label it `Mir X.Y`, keeping `X.Y.Z` in the destination URL.
  The landing page lists releases and status, not release content.

## Inputs to gather

1. **Templates and current content**: read
   `doc/sphinx/release-notes/template.include.md`, the page for the version being
   released, including earlier point-release sections, `.github/workflows/changelog-template`,
   and the current Debian entry.
   Use recent pages for tone, detail and MyST linking conventions.

1. **GitHub-generated release notes comment**: on the release PR, find the comment
   containing the `<!-- github-generated-release-notes -->` marker (posted by
   `Start Release`). Treat it as a PR checklist, not curated prose.

1. **Git history since the previous release**: find the previous release tag (the
   nearest ancestor tag not ending in `-rc`/`-dev`) and list commits/PR merges between
   it and the release branch tip.

   ```bash
   PREV_TAG=$(git describe --abbrev=0 --exclude="*-rc" --exclude="*-dev" <release-branch>)
   git log --oneline "${PREV_TAG}..<release-branch>"
   ```

1. **Diff since the previous release**: inspect the actual code changes to understand
   scope and user-visible impact, not just commit titles.

   ```bash
   git diff "${PREV_TAG}..<release-branch>"
   ```

## Steps

1. Gather the inputs above.
1. Cross-reference the generated notes' PR list against the commit log to catch any PRs
   whose titles don't clearly describe user-visible impact. Prefer the actual diff
   whenever a title is vague or understates the change.
1. Write explanatory, user-facing prose following the format above. Explain the
   previous limitation, new behavior, who benefits, and how to adopt or enable it
   when supported by the diff or documentation. Group related smaller changes and
   bug fixes together; provide more context than the concise packaging bullets.
1. Explain compatibility changes with affected interfaces, old and new behavior,
   replacements and concrete migration steps. Distinguish removals from deprecations;
   name removal versions only when confirmed. Describe build/runtime requirements
   only when supported by repository evidence. Never copy the example product's
   Python requirements, hardware recommendations or LTS/support promises into Mir.
1. Link to relevant documentation and PRs or issues, retaining contributor credits.
   Use MyST cross-references for local documentation. Keep partial protocol support
   explicitly qualified. Do not invent capabilities from protocol names or PR titles.
1. Summarize the same user-visible changes in the Debian entry under `Enhancements`,
   `Bugs fixed` and `Documentation`. Keep those bullets concise, with PR/issue numbers,
   preserving indentation, header and maintainer trailer. Update packaging ABI summary
   lines using the actual library versions and ABI changes in the diff; replace
   placeholders and remove unused categories.
1. Leave internal-only or purely mechanical changes (CI, tooling, refactors with no
   user-visible effect) out of the notes.

## Automation and scope

- Edit only the target point-release section and Debian entry, except for obsolete
  date removal. Leave landing-page status markers to the workflows.
- Never modify the Debian changelog entry header. Preserve its version (including
  suffixes and package revision), distribution/release state (such as `UNRELEASED`
  or `noble`) and urgency exactly. Version and release-state transitions belong to
  the release workflows; do not run `dch`/`debchange` to change them.
- Starting development creates a page from the include template and links it as
  in development. Prepare patch-release sections in that page before starting an RC.
  The RC step only changes `Unreleased` to `Release Candidate` and marks the existing
  landing link as release candidate; it does not create pages, sections or TOC entries.
- Finalization removes that status and replaces `Release Candidate` with the date,
  and publishes only the target section without Sphinx metadata or other releases.
  Keep these lifecycle operations inline in `awk`, without a separate helper script.
- For documentation, the merge driver reconciles only landing-page conflicts
  between the next development link and an incoming final release. Individual
  release pages use normal Git merging; genuine content conflicts must remain visible.

## Validation

Run focused Markdown formatting and documentation checks for the edited page,
including link checks. Use `python3 -m unittest tools.test_merge_release_entries -v`
for workflow/merge changes and run pre-commit hooks on the touched files.
Verify that no template comments, placeholders or documentation ABI summary remain.
Report any build or link checks that could not be run.
