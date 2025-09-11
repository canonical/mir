(mir-continuous-integration)=

# Mir Continuous Integration

This document outlines the journey of a contribution to Mir through its continuous integration
pipeline.

## Overview

**Diagram**: A flowchart depicting the steps in the Mir continuous integration process.

```{mermaid} continuous-integration.mmd
```

These are run at different stages in the pipeline, balancing the time it takes to run and the
breadth of testing. We'll discuss those in more detail below.

## Mir builds

When a pull request is opened, updated or merged into `main`, we validate that the contribution
is correct by building the code and running our test suite. To facilitate this, we use
[Spread](https://github.com/canonical/spread) (or rather, our
[lightly patched version](https://snapcraft.io/spread-mir-ci)) to build Mir across a number of
environments. [`spread.yaml`](https://github.com/canonical/mir/blob/main/spread.yaml) holds
environment definitions, while the actual build tasks are maintained under
[`spread/build`](https://github.com/canonical/mir/tree/main/spread/build).

Our focus of development is the most recent Ubuntu LTS, and we maintain builds for any more recent,
supported Ubuntu releases. We also build for stable releases of other Linux distributions that
we have community interest on.

We also run builds using alternative toolchains (Clang) and development versions of Ubuntu and
those other distributions.

We also maintain Mir branches for all previous Ubuntu LTS releases under support. These will only
receive security updates and the occasional bug fix. These will always be
[`release/` branches on GitHub](https://github.com/canonical/mir/branches/all?query=release%2F),
and releases published on [the GitHub release pages](https://github.com/canonical/mir/releases).

Opening a pull request, or merging into the `main` or `release/` branches, triggers a
[build on GitHub](https://github.com/canonical/mir/actions/workflows/spread.yml).

As part of the build, the following sets of tests are run:

### Unit tests

Defined in [`tests/unit-tests`](https://github.com/canonical/mir/tree/main/tests/unit-tests),
these assert the functionality of individual components in isolation, as is usual practice.

### Acceptance tests

[`tests/acceptance-tests`](https://github.com/canonical/mir/tree/main/tests/acceptance-tests)
run more high-level tests, ensuring we maintain the contracts with external components as well as
our own loadable modules' interfaces.

One major part of this is [wlcs](https://github.com/canonical/wlcs) - asserting the behavior
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

In addition to the above, for every merge to `main` we build and run the tests using the following
sanitizers:

- **Undefined Behavior**
  Building with UndefinedBehaviourSanitizer enabled reports no undefined behavior, it would be a
  CI failure otherwise.
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

Merges to `main`, `release/` branches as well as annotated tags are followed by `.deb` package
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

## End-to-end testing

When the snaps get published to the store, we have Jenkins dispatch test runs on a selection of
hardware, ranging from Raspberry Pis through single-GPU systems all the way up to high performance
multi-GPU (usually hybrid) ones. We maintain a matrix of test coverage
[in this spreadsheet](https://docs.google.com/spreadsheets/d/1kUbTSt4zWVpTtgZNJvvxCdugsRUv6C5PK9Xw5dxppCc/edit#gid=893560997).

These tests ultimately verify the full story end-to-end, installing the snaps on a range of
Ubuntu versions (Core and classic alike) and verify behaviors and visuals through a number of
scenarios.

**Diagram**: A flowchart depicting the end-to-end testing process for Mir.

```{mermaid} end-to-end-testing.mmd
```

[checkbox-mir](https://github.com/canonical/checkbox-mir/) is the test orchestrator, collecting
the different tests (smoke, performance and functional) and packaging them in a way that can be
consumed by our lab setup. It's using our own [mir-ci](https://github.com/canonical/mir-ci) tests
along with the [mir-test-tools](https://github.com/canonical/mir-test-tools) snap.

You can find more information about Checkbox itself
[in its documentation](https://github.com/canonical/checkbox) - it's a system used by our
certification team, running thousands of tests on hundreds of systems every day.

The Jenkins job definitions go into a private repository, as they contain credentials.
[jenkins-job-builder](https://pypi.org/project/jenkins-job-builder/) is used to maintain the jobs,
and at runtime, [testflinger](https://github.com/canonical/testflinger) dispatches them to the
device under test, while [test-observer](https://github.com/canonical/test_observer) collects the
results.

## Debugging

### GitHub Actions

When in need of hands-on debugging, our GitHub workflows have a ["Setup tmate" step](https://github.com/canonical/mir/blob/92fc772bc32f921c3a1cde7f17abb43a3d482f55/.github/workflows/spread.yml#L123-L127).
It allows for SSH access to the runner on failed runs. To use it, restart a failing job **with debug logging** enabled and wait for the step to fail. The [tmate action](https://github.com/marketplace/actions/debugging-with-tmate) will then repeatedly log:

```
SSH: ssh <id>@<host>
or: ssh -i <path-to-private-SSH-key> <id>@<host>
```

Use the provided `ssh` command to access the runner. You'll be dropped in the default working directory and environment. Use "Ctrl+B", "c" to create a new window within the tmate session - see [tmate's man page](https://manpages.ubuntu.com/manpages/noble/man1/tmate.1.html#key%20bindings) for more key bindings.

#### Spread

When Spread is used, the actual build runs in LXD containers, use `lxc ls` and `lxc shell <container>` to get inside.

#### Sbuild

Sbuild adds another layer, a chroot environment. Use `schroot --list --all-sessions` to list the available sessions and `schroot --run --chroot session:<id>` to access it. The build is in `/build`. To run many Mir tests you'll need to `export XDG_RUNTIME_DIR=/tmp`.

## Summary

As shown above, Mir is being tested quite extensively on a large number of environments. We're
extending the coverage every day, as well. There's a lot of moving pieces involved, but it does
help us greatly in ensuring the quality level we hold ourselves to.
