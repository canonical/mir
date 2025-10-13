(how-to-test-mir-for-a-release)=

# How to test Mir for a release

The following document details the criteria that must be met before we can
publish an official release of Mir. When adding to this test plan, please
do not fail to consider how your additions may be automated in the future, as
this is the end goal of our release testing.

Note that all of these tests must be run _after_ a release branch is created
in the Mir repository, such that the repository is updated beforehand.

## Setup

We will run the test plan on the `miral-app` test compositor.

1. Install mir from the release candidate PPA
   ```sh
   sudo add-apt-repository --update ppa:mir-team/rc
   sudo apt install mir-demos mir-platform-graphics-virtual mir-graphics-drivers-desktop mir-test-tools
   ```
1. Confirm that you can run `miral-app`

## Testing with `mir-smoke-test-runner`

These verify our demo clients work with `mir_demo_server` and should be run for
each of the ubuntu series (see previous section).

The tests will automatically test the platforms that are expected to work.

1. Run `mir-smoke-test-runner` like so:

   ```sh
   mir-smoke-test-runner
   ```

1. Confirm that the final output of the test begins with: `Smoke testing complete with returncode 0`.

The platforms tested will be visible at the end of the line. For example:

```
I: Smoke testing complete with returncode 0, platforms tested: virtual x11 wayland
```

These will differ between running "hosted" (from a terminal window) and running
"native" (from a virtual terminal). They should be run both ways and the
results collated.

## Testing with `mir-compositor-smoke-test`

These verify our demo servers work and should be run for each of the ubuntu
series (see previous section).

1. Run `mir-compositor-smoke-test` like so:

   ```sh
   mir-compositor-smoke-test
   ```

1. Confirm that the output of the test ends with: `I: compositor testing complete with returncode 0`.
   The preceding line confirms the compositors tested. E.g.

   ```plain
   I: The following compositors executed successfully: /usr/bin/miral-system-compositor /usr/bin/miral-kiosk /usr/bin/miral-shell
   ```

## Testing each graphics platform

Each test must be performed across a combination of different display
platforms and Ubuntu releases. The following matrix provides the environments
in which we need to test:

|                                | 24.04 | 25.04 |
| ------------------------------ | ----- | ----- |
| gbm-kms                        |       |       |
| eglstream-kms                  |       |       |
| eglstream-kms + gbm-kms hybrid |       |       |
| x11                            |       |       |
| wayland                        |       |       |
| virtual                        |       |       |

To check which display platform we've selected, we can run `miral-app`
and grep for the platform string as follows:

```sh
miral-app | grep "Selected display driver:"
```

Given the types of outputs that you have configured in your environment,
you should encounter one of the following scenarios for each output:

1. When NOT on an Nvidia platform and NOT in a hosted environment,
   then `mir:gbm-kms` is selected

1. When you have an Nvidia card connected to an output _and_ the system
   is using Nvidia's proprietary drivers, then `mir:eglstream-kms`
   is selected

1. When you are running the compositor hosted in a session that supports X11,
   then `mir:x11` is selected

1. When you are running the compositor hosted in a session that supports wayland
   _and_ you force Mir to use the `mir:wayland` platform using:

   ```
   miral-app --wayland-host=$WAYLAND_DISPLAY
   ```

   then `mir:wayland` is selected.

1. Check that the virtual platform can run and you can connect to it via a VNC:

   ```sh
   MIR_SERVER_PLATFORM_DISPLAY_LIBS=mir:virtual MIR_SERVER_VIRTUAL_OUTPUT=1280x1024 WAYLAND_DISPLAY=wayland-1 miral-app \
       --add-wayland-extension=zwp_virtual_keyboard_manager_v1:zwlr_virtual_pointer_manager_v1:zwlr_screencopy_manager_v1
   ```

   After, in a separate VT, connect to the VNC:

   ```sh
   WAYLAND_DISPLAY=wayland-1 ubuntu-frame-vnc
   ```

   Then, check that you can connect via a VNC viewer:

   ```sh
   gvncviewer localhost
   ```

## Applications

For each empty box in the matrix above, ensure that the following applications can start

1. Test that the following QT Wayland applications can be started and can receive input:

   ```sh
   sudo apt install qtwayland5 kate qterminal

   # First...
   kate

   # Then...
   qterminal
   ```

1. Test that `weston-terminal` can be started and can receive input:

   ```
   sudo apt install weston-terminal
   weston-terminal
   ```

1. Test that `glmark2-wayland` can be run:

   ```sh
   sudo apt install glmark2-wayland
   glmark2-wayland
   ```

1. (If using gbm-kms on a system with multiple GPUs) Test hybrid support with `glmark2-wayland`

   ```sh
   sudo apt install glmark2-wayland
   glmark2-wayland
   DRI_PRIME=0 glmark2-wayland
   DRI_PRIME=1 glmark2-wayland
   ```

   (If more than 2 GPUs, may do `DRI_PRIME=2 glmark2-wayland`, etc)

1. Test that `gnome-terminal` can be started and can receive input:

   ```sh
   sudo apt install gnome-terminal
   gnome-terminal
   ```

1. Test that `X11` apps can be started and can receive input:

   ```sh
   sudo apt install x11-apps

   # first,
   xeyes

   # then
   xedit

   # finally
   xcalc
   ```

## Mir console providers

For each Ubuntu release ensure that the compositor can start with each of the console providers:

|         | 24.04 | 25.04 |
| ------- | ----- | ----- |
| vt      |       |       |
| logind  |       |       |
| minimal |       |       |

The following describes how to select each console provider:

1. **vt**:

   ```sh
   miral-app --console-provider=vt --vt=4
   ```

   - This requires running with root privileges
   - You need to ensure that `XDG_RUNTIME_DIR` is set in the environment. If using `sudo`,
     it might strip this out; running something like `sudo env XDG_RUNTIME_DIR=/run/user/1000 miral-shell ...`
     will ensure this is set.

1. **logind**:

   ```sh
   miral-app --console-provider=logind
   ```

   - This requires running from a logged-in VT where `logind` can provide DRM :woke-ignore:`master`
   - So, first switch to vt4 and sign in
   - This can be used by
     - the "hardware" platforms: `gbm-kms`, `eglstream-kms`, and `atomic-kms`; and,
     - the `virtual` platform

1. **minimal**:

   ```sh
   miral-app --console-provider=minimal
   ```

   - This is used when all others fail
   - This does not provide VT switching capabilities (Ctrl-Alt-F1, etc)
   - This is _only_ used for the `gbm-x11`, `gbm-wayland`, and `virtual` platforms

## Window manager examples

Run with different window managers and confirm that the window management
styles work as expected:

```sh
miral-app --window-manager=floating # traditional floating window manager
miral-app --window-manager=tiling # a tiling window manager
miral-app -kiosk # a typical kiosk
```

## Testing downstream snaps (e.g. Ubuntu Frame and Miriway)

For each of our downstream snaps, check that you have installed a build with the Mir version under test (typically from the `beta` channel). Then run the tests for that snap.

E.g. for `mir-test-tools`:

```sh
/snap/mir-test-tools/current/bin/selftest
```
