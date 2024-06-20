# Mir Continuous Integration

This document outlines the journey of a contribution to Mir through its continuous integration
pipeline.

## Overview

There are a number of components to this story:
- Mir builds
- unit, integration, acceptance and performance tests
  - including our [WayLand Conformance Suite - WLCS](https://github.com/canonical/wlcs)
- sanitizer runs
- ABI checks
- coverage measurement
- deb package builds
- downstream Snap builds
- end-to-end testing across different hardware
  - test procedures maintained within [mir-ci](https://github.com/canonical/mir-ci)
    and [checkbox-mir](https://github.com/canonical/checkbox-mir)
  - automations defined in a private repository, as it includes infrastructure credentials

These are run at different stages in the pipeline, balancing the time it takes to run and the
breadth of testing. We'll discuss those in more detail below.

### Mir builds

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

### Unit tests

Defined in [`tests/unit-tests`](https://github.com/canonical/mir/tree/main/tests/unit-tests),
these assert the functionality of individual components in isolation, as is usual practice. They
are run for every build above.

### Acceptance tests

[`tests/acceptance-tests`](https://github.com/canonical/mir/tree/main/tests/acceptance-tests)
similarly are run for every build above, and run more high-level tests, ensuring we maintain the
contracts with external components as well as our own loadable modules' interfaces.

One major part of this is [WLCS](https://github.com/canonical/wlcs) - asserting the behaviour
of Mir's Wayland frontend is to specification of the protocol.
