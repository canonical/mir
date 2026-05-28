(how-to-release-a-new-mir-version)=

# How to release a new Mir version

## Versions 2.26 and later

Mir releases are now driven by two GitHub Actions workflows:

- [`Start Release`](https://github.com/canonical/mir/blob/main/.github/workflows/start-release.yml)
- [`Finalize Release`](https://github.com/canonical/mir/blob/main/.github/workflows/finalize-release.yml)

This guide explains when to run each workflow and what to verify around them.

### Embargoed security remediations

If releasing an embargoed security fix,
perform this in a dedicated private repository before merging into `main` and making the GitHub release once the embargo lifts.

### Start a release candidate

Run the [Start Release](https://github.com/canonical/mir/actions/workflows/start-release.yml) workflow from GitHub Actions:

- `version` (required): the target release version (for example, `2.26.1`)
- `sha` (optional): commit-ish to release, defaults to the selected ref tip (this can also be a tag for which a patch release is being prepared)

This workflow prepares the release candidate across git, packaging and GitHub.
If the new release is started from `main` without a custom `sha`, the next development release will also be bootstrapped on `main`.

After it completes, verify:

1. A `release/X.Y` branch exists and contains the RC changes.
1. A `vX.Y.Z-rc` tag exists.
1. A draft GitHub release for `vX.Y.Z` exists.
1. The release PR was created when run from `main`.

Then run the release testing in [](how-to-test-mir-for-a-release).

### Prepare the release notes

Edit [](release-notes) on the release branch to include crafted notes for relevant changes included in the release.
You can use [the draft GitHub release](https://github.com/canonical/mir/releases/) as reference.

### Cherry-pick any needed changes

Use `git cherry-pick --mainline 1 <ref>` to commit any changes to the release branch.

### Finalize the release

After RC testing passes, run [Finalize Release](https://github.com/canonical/mir/actions/workflows/finalize-release.yml):

- `release_branch` (required): release branch name, for example `release/2.26`

This workflow finalizes the release across git, packaging and GitHub.

After it completes, verify:

1. Tag `vX.Y.Z` exists on the release branch tip.
1. The GitHub release is published (no longer draft).
1. The release notes body on GitHub and in packaging matches the extracted section from [](release-notes).

### Merge the release branch into `main`

Finally, manually merge and push the release branch into `main`,
resolving conflicts to ensure the correct ordering and dates in [](release-notes), `debian/changelog` and `rpm/mir.spec`.

## Versions before 2.26

Before these workflows were introduced, releases were performed manually.
At a high level, the manual process mirrored what the workflows now automate:

1. Create/update `release/X.Y` from the last patch release on the series.
1. Update [](release-notes), `CMakeLists.txt`, `debian/changelog` and `rpm/mir.spec` to the target release.
1. Commit any appropriate changes to the release branch and
   - push to the origin
   - or if it's an embargoed change:
     - push to the dedicated repository
     - modify `tools/ppa-upload.sh` to point at the dedicated PPA rather than `~mir-team/rc`
     - run `RELEASE=XX.YY tools/ppa-upload.sh` to upload to the relevant PPA
1. If not embargoed, open a Pull Request into `main`.
1. Run release testing and collect sign-off.
1. Commit, sign, and tag `vX.Y.Z`; push to the appropriate targets as above.
1. If not embargoed, or once that's lifted:
   1. Merge the release branch into `main`.
   1. Publish the GitHub release.
