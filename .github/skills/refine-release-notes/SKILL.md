---
name: refine-release-notes
description: Refine the debian/changelog entry for an in-progress Mir release using the changelog template, the GitHub-generated release notes comment on the release PR, and the git history and diff since the previous release.
---

# Refine Release Notes

Use this workflow after `Start Release` has opened the release PR, to turn the raw
GitHub-generated release notes into a crafted entry in `debian/changelog`.
`.github/workflows/changelog-template` defines the structure, and `debian/changelog`
is the authoritative release content. `Finalize Release` copies that content to the
RPM spec changelog and the final GitHub release body. Do not edit
`doc/sphinx/release-notes.md` as part of this workflow.

## Inputs to gather

1. **Changelog template and current entry**: read
   `.github/workflows/changelog-template` and the entry for the version being
   released in `debian/changelog`. Use the template for the categories and ABI
   summary, and previous Debian entries for tone and formatting.

1. **GitHub-generated release notes comment**: on the release PR, find the comment
   containing the `<!-- github-generated-release-notes -->` marker (posted by
   `Start Release`). This lists merged PRs since the previous release, grouped by
   contributor, but is not curated prose.

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
   whose titles don't clearly describe user-visible impact; read the diff for those.
1. Classify each user-visible change into the changelog template's categories:
   `Enhancements`, `Bugs fixed`, `Documentation`, alongside the ABI summary.
1. Write concise, user-facing bullet points (not raw PR titles) referencing the PR or
   issue number, following the format already used in `debian/changelog` for prior
   releases. Preserve Debian bullet indentation, the entry header and the maintainer
   trailer; replace template placeholders and remove unused categories.
1. Update the ABI summary lines to reflect whether each library's ABI was bumped or is
   unchanged, based on `symbols.map`/`debian/*.symbols` changes in the diff.
1. Leave internal-only or purely mechanical changes (CI, tooling, refactors with no
   user-visible effect) out of the notes.
1. Edit only the release entry in `debian/changelog`. Do not edit `rpm/mir.spec` or
   the GitHub release body directly; `Finalize Release` generates them from the
   Debian entry.

## Rules

- Keep edits scoped to the section for the version being released.
- Never modify the Debian changelog entry header. Preserve its version (including
  suffixes and package revision), distribution/release state (such as `UNRELEASED`
  or `noble`) and urgency exactly. Version and release-state transitions belong to
  the release workflows; do not run `dch`/`debchange` to change them.
- Prefer the diff over the PR title whenever a title is vague or a change has
  broader/narrower impact than its title suggests.
- Match the existing tone and structure of previous entries in `debian/changelog`,
  using `.github/workflows/changelog-template` as the structural authority.
