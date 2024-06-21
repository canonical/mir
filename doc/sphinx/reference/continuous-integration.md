# Mir Continuous Integration

This document outlines the journey of a contribution to Mir through its continuous integration
pipeline.

## Overview

There are a number of components to this story, and a diagram might make for a good overview:

```{mermaid} continuous-integration.mmd
```

These are run at different stages in the pipeline, balancing the time it takes to run and the
breadth of testing. We'll discuss those in more detail below.

## Mir builds

We use [Spread](https://github.com/snapcore/spread) (or rather, our
[lightly patched version](https://snapcraft.io/spread-mir-ci)) to build Mir across a number of
environments. These are defined in
[`spread.yaml`](https://github.com/canonical/mir/blob/main/spread.yaml), with the actual build
tasks maintained under [`spread/build`](https://github.com/canonical/mir/tree/main/spread/build).

Our focus of development is the most recent Ubuntu LTS, and we maintain builds for any more recent,
supported Ubuntu releases. We also build for stable releases of other Linux distributions that
we have community interest on.

Additionally, there are builds using alternative toolchains (Clang) and development versions of
Ubuntu and those other distributions.

We also maintain Mir branches for all previous Ubuntu LTS releases under support. These will only
receive security updates and the occasional bug fix. These will always be
[`release/` branches on GitHub](https://github.com/canonical/mir/branches/all?query=release%2F),
and releases published on [the GitHub release pages](https://github.com/canonical/mir/releases).

All those builds [run on GitHub](https://github.com/canonical/mir/actions/workflows/spread.yml),
triggered by pull requests and pushes to `main` or `release/` branches.

As part of the build, the following sets of tests are run:

### Unit tests

Defined in [`tests/unit-tests`](https://github.com/canonical/mir/tree/main/tests/unit-tests),
these assert the functionality of individual components in isolation, as is usual practice.

### Acceptance tests

[`tests/acceptance-tests`](https://github.com/canonical/mir/tree/main/tests/acceptance-tests)
run more high-level tests, ensuring we maintain the contracts with external components as well as
our own loadable modules' interfaces.

One major part of this is [WLCS](https://github.com/canonical/wlcs) - asserting the behaviour
of Mir's Wayland frontend is to specification of the protocol.

### Integration tests

In [`tests/integration-tests`](https://github.com/canonical/mir/tree/main/tests/integration-tests)
lie other high level tests, ensuring we correctly integrate with some system components.

### Performance and smoke tests

Lastly, [`tests/performance-tests`](https://github.com/canonical/mir/tree/main/tests/performance-tests)
holds a handful of performance-related tests, collecting some metrics about how the system
performs end-to-end - and verifying that it works in the first place.

We also run these on a number of hardware platforms in our testing lab for every build of `main`.

## Sanitizer runs

In addition to the above, for every push to `main` we build and run the tests using the following
sanitizers:

- **Undefined Behaviour**
  This is now enforced, and no undefined behaviour is reported by ubsan.
- **Address Sanitizer**
  As we have some fixing to do here, we run those builds - but don't enforce the problems reported.
- **Thread Sanitizer**
  Unfortunately due to
  [incompatibility between GLib and TSan](https://github.com/google/sanitizers/issues/490), we
  don't currently run the thread sanitizer, as that produces too many false positives.

## ABI checks

For each pull request we
[check that the exported symbols are as expected](https://github.com/canonical/mir/actions/workflows/symbols-check.yml)
- and fail if the ABI changed in any way. It doesn't necessarily mean an ABI break - but new symbols
need to be tracked.

## Coverage measurement

We [track test coverage](https://github.com/canonical/mir/actions/workflows/coverage.yml) for each pull
request and `main` builds, and the results are visible on
[Codecov.io](https://app.codecov.io/gh/canonical/mir).

## `.deb` package builds

Pushes to `main`, `release/` branches as well as annotated tags are followed by `.deb` package
builds in mir-team's Launchpad's Personal Package Archives (PPAs):
- [`~mir-team/dev`](https://launchpad.net/~mir-team/+archive/ubuntu/dev) for the latest development
  builds
- [`~mir-team/rc`](https://launchpad.net/~mir-team/+archive/ubuntu/rc) for release candidate and
  release builds

At release, those get copied to the
[`~mir-team/release`](https://launchpad.net/~mir-team/+archive/ubuntu/release), general availability
PPA.

## Downstream snap builds

We [build a subset](https://github.com/canonical/mir/actions/workflows/snap.yml) of the downstream
snaps on pull requests - these are then available in `edge/mir-pr<number>`
[Snap channels](https://snapcraft.io/docs/channels) for testing, e.g.:

```shell
snap install miriway --channel edge/mir-pr1234
```

When `.deb` packages build in the PPAs above, all the affected downstream snaps get rebuilt through
the [`~mir-team` snap recipes on Launchpad](https://launchpad.net/~mir-team/+snaps) and made
available in the `edge` or `beta` channels, as appropriate.
