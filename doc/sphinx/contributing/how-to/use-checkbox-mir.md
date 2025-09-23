(how-to-use-checkbox-mir)=

# Using checkbox-mir to validate your snap graphical environment

This document explains how to use checkbox-mir to validate the graphical environment (drivers, userspace, snap setup) on your device.

## The graphics-coreXX interface

To support graphics on Linux on a given piece of hardware, usually a kernel and userspace driver is required. When using snaps, the kernel driver is managed by the operating system, but the userspace driver needs to be available to your application.

The recommended approach for this is using a content snap, through the appropriate content interface. The current iteration is `gpu-2404`, see here for more information on that setup: [The gpu-2404 interface](https://canonical.com/mir/docs/the-gpu-2404-snap-interface).

## Checkbox

Checkbox is a test automation software we're using extensively at Canonical. We chose it as a framework for providing these tests so that it's easy to run without specific knowledge.

You can find out more about Checkbox on its GitHub page: [Checkbox](https://github.com/canonical/checkbox/).

## checkbox-mir

[checkbox-mir](https://snapcraft.io/checkbox-mir) is a suite of tests maintained by the Mir team that range from validating the DRM setup through to performance testing of Mir servers. It's provided as a snap package, so on most Linux distributions it's enough to:

```text
$ sudo snap install checkbox-mir --devmode
checkbox-mir 0+git.7970755 from Canonical Certification Team (ce-certification-qa) installed
```

The `--devmode` flag is required for Checkbox to be able to perform the testing.

You can find out more about it provides through the `snap info` command:

```text
$ snap info checkbox-mir
name:      checkbox-mir
summary:   Checkbox tests for the Mir project
publisher: Canonical Certification Team (ce-certification-qa)
store-url: https://snapcraft.io/checkbox-mir
license:   unset
description: |
  Collection of tests to be run on devices that are part of the mir project
commands:
  - checkbox-mir.checkbox-cli
  - checkbox-mir.graphics
  - checkbox-mir.mir
  - checkbox-mir.shell
  - checkbox-mir.snaps
  - checkbox-mir.test-runner
services:
  checkbox-mir.service: simple, enabled, active
snap-id:      zPeO6SzpA2i39SNFjpm8qAkMPTOMvMxh
tracking:     latest/stable
refresh-date: today at 12:04 CEST
channels:
  latest/stable:    0+git.7970755 2023-07-20 (49) 17MB -
  latest/candidate: ↑
  latest/beta:      ↑
  latest/edge:      0+git.7970755 2023-07-19 (49) 17MB -
installed:          0+git.7970755            (49) 17MB devmode
```

### Prerequisites

Depending on the test being ran, there are different requirements for it to start, and the test framework will try and determine if it's possible to run the test, rather than fail it.

Here's a list of the different requirements:

1. [`graphics-test-tools`](https://snapcraft.io/graphics-test-tools) installed,
1. [`mir-test-tools`](https://snapcraft.io/mir-test-tools) installed,
1. the graphics userspace provider snaps installed (by default that's [mesa-2404](https://snapcraft.io/mesa-2404) providing `gpu-2404`, depending on which track of the above you install), and the above snaps, and `checkbox-mir` itself connected to them,
1. to run as a non-privileged user and confirm Mir interactions with the display manager, a local user session needs to be active,

### `checkbox-mir.checkbox-cli`

`checkbox-mir.checkbox-cli` is the interactive entry point, in which you can manually select the test plan and tests to run, as well as control some aspects of the run:

```text
 Select test plan
┌───────────────────────────────────────────────────────────────────────┐
│                                                                       │
│    ( ) Mir - smoke and performance testing of Mir itself              │
│    ( ) Snap - verify that the snaps work fine                         │
│    ( ) graphics-core - verify the graphics-core enablement            │
│                                                                       │
│                                                                       │
│                                                                       │
│                                                                       │
│                                                                       │
│                                                                       │
│                                                                       │
└───────────────────────────────────────────────────────────────────────┘
 Press <Enter> to continue                                      (H) Help
```

See [Checkbox documentation](https://canonical-checkbox.readthedocs-hosted.com/latest/) for more information on the framework itself and how to interact with it.

### The graphics-core test plan

The graphics-coreXX test plan attempts to verify the graphics-coreXX enablement by running a handful of utilities verifying a number of aspects of graphics support:

- DRM
- KMS
- eglinfo across GBM, Wayland and X
- libVA
- VDPAU
- Vulkan

```text
 Choose tests to run on your system:
┌───────────────────────────────────────────────────────────────────────┐
│[X] - Uncategorised                                                    │
│[X]    Collect information about connections                           │
│[X]    Enumerate available system executables                          │
│[X]    Dump DRM information for card 1: drm-pci-0000_00_02_0           │
│[X]    Run kmscube to verify GBM-KMS setup for card 1:                 │
│       drm-pci-0000_00_02_0                                            │
│[X]    Run eglinfo to verify EGL setup for the GBM platform            │
│[X]    Run eglinfo to verify EGL setup with a Wayland server           │
│[X]    Run eglinfo to verify EGL setup with a X server                 │
│[X]    Run vainfo to verify libVA setup                                │
│[X]    Run vdpauinfo to verify VDPAU setup                             │
│[X]    Run vkcube to verify Vulkan setup                               │
│                                                                       │
│                                                                       │
└───────────────────────────────────────────────────────────────────────┘
 Press (T) to start Testing                                     (H) Help
```

The log results from each of these tests should provide insight into the status of the aspect in question. It's out of scope for this document to discuss the results beyond their success / failure state.

### The Mir test plan

This test plan runs the smoke and performance tests as found in the [mir-test-tools](https://snapcraft.io/mir-test-tools) snap. This is what we use to verify updated snaps continue to work across a number of hardware and scenarios.

```text
 Choose tests to run on your system:
┌───────────────────────────────────────────────────────────────────────┐
│[X] - Uncategorised                                                    │
│[X]    Collect information about connections                           │
│[X]    Hardware Manifest                                               │
│[X]    Provide information about logind sessions                       │
│[X]    Run Mir performance test case:                                  │
│       CompositorPerformance.regression_test_1563287                   │
│[X]    Collect the test report from                                    │
│       mir/performance_CompositorPerformance.regression_test_1563287   │
│[X]    Run Mir performance test case: GLMark2Wayland.fullscreen        │
│[X]    Collect the test report from                                    │
│       mir/performance_GLMark2Wayland.fullscreen                       │
│[X]    Run Mir performance test case: GLMark2Wayland.windowed          │
│[X]    Collect the test report from                                    │
│       mir/performance_GLMark2Wayland.windowed                         │
│[X]    Run Mir performance test case: GLMark2Xwayland.fullscreen       │
│                                                                       │
└───────────────────────────────────────────────────────────────────────┘
 Press (T) to start Testing                                     (H) Help
```

After selecting the tests, there's another step that we use to skip tests we know fail on a given piece of hardware:

```text
System Manifest:
┌───────────────────────────────────────────────────────────────────────┐
│ Does this machine have this piece of hardware?                        │
│   Run Hosted tests                   ( ) Yes   ( ) No                 │
│   Run Wayland tests                  ( ) Yes   ( ) No                 │
│   Run Xwayland tests                 ( ) Yes   ( ) No                 │
│                                                                       │
│                                                                       │
│                                                                       │
│                                                                       │
│                                                                       │
└───────────────────────────────────────────────────────────────────────┘
 Press (T) to start Testing                               Shortcuts: y/n
```

### The snap test plan

This test plan is primarily intended for the Mir team, allowing us to test updates to the snaps we maintain:

```text
 Choose tests to run on your system:
┌───────────────────────────────────────────────────────────────────────┐
│[X] - Uncategorised                                                    │
│[X]    snaps/confined-shell-edge                                       │
│[X]    snaps/egmde-beta                                                │
│[X]    snaps/mir-kiosk-kodi-edge                                       │
│[X]    snaps/mir-kiosk-neverputt-edge                                  │
│[X]    snaps/mir-kiosk-scummvm-edge                                    │
│[X]    snaps/mir-test-tools-20-beta                                    │
│[X]    snaps/mir-test-tools-22-beta                                    │
│[X]    snaps/mir-test-tools-24-beta                                    │
│[X]    snaps/mircade-beta                                              │
│[X]    snaps/miriway-beta                                              │
│[X]    snaps/ubuntu-frame-osk-20-beta                                  │
│[X]    snaps/ubuntu-frame-osk-22-beta                                  │
│[X]    snaps/ubuntu-frame-osk-24-beta                                  │
└───────────────────────────────────────────────────────────────────────┘
 Press (T) to start Testing                                     (H) Help
```

### Launchers

We provide a couple of launchers for automated runs:

- `checkbox-mir.graphics`: runs the graphics-core test plan
- `checkbox-mir.mir`: runs the Mir test plan

If you want to integrate checkbox-mir into your CI, these are likely what you want to run after having set your device up. You may also create a custom launcher, see {ref}`checkbox:launcher`.

`checkbox-mir.snaps` preselects the Snap test plan, but lets you choose which snaps to test.

### Running remotely

The recommended way to run Checkbox tests is over the network - and using Checkbox itself, rather than a SSH connection (see {ref}`checkbox:remote` for more information).

To do this, install checkbox-mir and dependencies on the target device as you would normally, but rather than running the tests there, install Checkbox on your host and issue `checkbox remote <ip.address>`, pointing at the device you want to test.

You'll see the same user interface, be able to run the tests as you would locally - but from a distance, without impacting the test results by the environment you run them from etc. The test results will also be brought into your host at the end.

______________________________________________________________________

We'd love your feedback, please come to https://github.com/canonical/checkbox-mir and file issues, pull and enhancement requests. This test suite will grow for sure, and we'd love to know if you think things could be better.
