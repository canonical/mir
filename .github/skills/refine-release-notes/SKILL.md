---
name: refine-release-notes
description: Refine the {ref}`release-notes` entry for an in-progress Mir release by combining the GitHub-generated release notes comment on the release PR with the git history and diff since the previous release.
---

# Refine Release Notes

Use this workflow after `Start Release` has opened the release PR, to turn the raw
GitHub-generated release notes into a crafted entry in `doc/sphinx/release-notes.md`.
`doc/sphinx/release-notes.md` is the single source of truth: the Debian changelog, RPM
spec changelog and the final GitHub release body are all generated from it by
`Finalize Release`, so it is the only file that needs manual editing here.

## Inputs to gather

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

1. Gather the three inputs above.
1. Cross-reference the generated notes' PR list against the commit log to catch any PRs
   whose titles don't clearly describe user-visible impact; read the diff for those.
1. Classify each user-visible change into the existing `release-notes.md` categories:
   `Enhancements`, `Bugs fixed`, `Documentation` (see the template comment at the top of
   the file for the full structure, including the ABI summary).
1. Write concise, user-facing bullet points (not raw PR titles) referencing the PR or
   issue number, following the format already used in `doc/sphinx/release-notes.md` for
   prior releases.
1. Update the ABI summary lines to reflect whether each library's ABI was bumped or is
   unchanged, based on `symbols.map`/`debian/*.symbols` changes in the diff.
1. Leave internal-only or purely mechanical changes (CI, tooling, refactors with no
   user-visible effect) out of the notes.
1. Do not edit `debian/changelog`, `rpm/mir.spec`, or the GitHub release body directly —
   `Finalize Release` regenerates them from `doc/sphinx/release-notes.md`.

## Rules

- Keep edits scoped to the section for the version being released.
- Prefer the diff over the PR title whenever a title is vague or a change has
  broader/narrower impact than its title suggests.
- Match the existing tone and structure of previous entries in `release-notes.md`.
