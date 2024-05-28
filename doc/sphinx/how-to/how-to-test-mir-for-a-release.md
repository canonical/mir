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
2. Confirm that you can run `miral-app`

## Tests
Each test must be performed across a combination of different display
platforms and different Ubuntu releases. The following matrix provides
the environments in which we need to test:

|                                | 24.04    | 24.10      |
|--------------------------------|----------|------------|
| gbm-kms                        |          |            |
| eglstream-kms                  |          |            |
| eglstream-kms + gbm-kms hybrid |          |            |
| x11                            |          |            |
| wayland                        |          |            |
| virtual                        |          |            |

### Platform Selection
The first set of tests confirms that platforms are selected appropriately.

> Note that this section should NOT be run for each platform
> by nature. The remaining test sections _**should**_ be run for each platform
> in the matrix.

To verify the platform in each instance, do:

```
miral-app | grep "Selected display driver:"
```

The tests are as follows:

1. When NOT on an Nvidia platform and NOT in a hosted environment,
   then `mir:gbm-kms` is selected
2. When you have an Nvidia card connected to an output _and_ the system
   is using Nvidia's proprietary drivers, then `mir:eglstream-kms`
   is selected
3. When you are running the compositor hosted in a session that supports X11,
   then `mir:x11` is selected
4. When you have an Nvidia card connected to an output _and_ the system
   is using Nvidia's proprietary drivers, you may force Mir to use the
   `mir:gbm-kms` platform using:
   ```
   miral-app --platform-display-libs=mir:gbm-kms
   ```
   Verify that `mir:gbm-kms` is selected.
5. When you are running the compositor hosted in a session that supports wayland
   _and_ you force Mir to use the `mir:wayland` platform using:
   ```
   miral-app --platform-display-libs=mir:wayland --wayland-host=$WAYLAND_DISPLAY
   ```
   then `mir:wayland` is selected.

6. Check that the virtual platform can run and you can connect to it via a VNC:
   ```sh
   WAYLAND_DISPLAY=wayland-1 miral-app --platform-display-lib=mir:virtual \
       --virtual-output=1280x1024 \
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

### Applications
For each empty box in the matrix above, ensure that the following applications can start

1. Test that the following QT Wayland applications can be started and can receive input:
    ```sh
    sudo apt install qtwayland5 kate qterminal
    
    # First...
    kate
   
    # Then...
    qterminal
    ```
2. Test that `weston-terminal` can be started and can receive input:
    ```
    sudo apt install weston
    weston
    ```
3. Test that `glmark2-wayland` can be run:
    ```sh
    sudo apt install glmark2-wayland
    glmark2-wayland
    ```
4. Test that `gnome-terminal` can be started and can receive input:
   ```sh
   sudo apt install gnome-terminal
   gnome-terminal
   ```


5. Test that `X11` apps can be started and can receive input:
    ```sh
    sudo apt install x11-apps
    
    # first,
    xeyes

    # then
    xedit

    # finally
    xcalc
    ```

### Mir Console Providers
Run with different console providers and ensure that the compositor can start:

```sh
miral-app --console-provider=vt --vt=4
```
- This requires running with root privileges 
- You need to ensure that `XDG_RUNTIME_DIR` is set in the environment. If using `sudo`, 
    it might strip this out; running something like `sudo env XDG_RUNTIME_DIR=/run/user/1000 miral-shell ...`
    will ensure this is set. 

```sh
miral-app --console-provider=logind
```
- You can switch to vt4 and sign in

```sh
miral-app --console-provider=minimal 
```
- This is used when all others fail
- This does not provide VT switching capabilities (Ctrl-Alt-F1, etc) 
- This is _only_ used for the `gbm-x11`, `gbm-wayland`, and `virtual` platforms 

### Window Manager Examples
Run with different window managers and confirm that the window management
styles work as expected:

```sh
miral-app --window-manager=floating # traditional floating window manager
miral-app --window-manager=tiling # a tiling window manager
miral-app -kiosk # a typical kiosk
```

### Run Compositors that are Built for Testing
1. Run the `mir-test-smoke-runner` and confirm that the final output is:
   `Smoke testing complete with returncode 0`.
2. Run the tests found in `mir-test-tools` and confirm the contents on the screen. A useful script for doing this is:
   ```bash
    #! /bin/bash

    set -e
   
    if [ -x "$(command -v fgconsole)" ]
    then
        trap "sudo chvt $(sudo fgconsole)" EXIT
    fi
   
    if [ -v DISPLAY ]
    then
        snap run mir-test-tools.gtk3-test
        snap run mir-test-tools.qt-test
        snap run mir-test-tools.sdl2-test
        snap run mir-test-tools.smoke-test
        snap run mir-test-tools.performance-test
        snap run mir-test-tools.mir-flutter-app
    fi
   
    sudo snap run mir-test-tools.gtk3-test
    sudo snap run mir-test-tools.qt-test
    sudo snap run mir-test-tools.sdl2-test
    sudo snap run mir-test-tools.smoke-test
    sudo snap run mir-test-tools.performance-test
    sudo snap run mir-test-tools.mir-flutter-app
   
    echo All tests passed
   ```

### Window Management
- Confirm that clip areas work for windows. This issue was first encountered in
  [#3201](https://github.com/canonical/mir/issues/3201). To test:
    1. Create a test compositor that clips a window to only contain its rectangle
    2. Move that window all over the screen and confirm that it remains properly clipped

### Misc
- Confirm that the system is restored properly after a wake. This issue was first
  encountered in [#3238](https://github.com/canonical/mir/issues/3238). To test:
    1. Have a setup with two monitors
    2. Configure the display so that it is set to clone:
       ```
       miral-app --display-config=clone
       ```
    2. Sleep your session
    3. Wake your session up
    4. Confirm that the compositor is still running and that the contents on the
       screen(s) match what should be there
    
    This would be automated by https://github.com/canonical/mir-ci/issues/126.

- Confirm that cursor speeds are consistent. This issue was first encountered in
  [#3205](https://github.com/canonical/mir/issues/3205). To test:
    1. Move the cursor around the screen
    2. Confirm that the cursor does not experience significant slow down

    This would be automated by https://github.com/canonical/mir-ci/issues/127.

- Confirm that the keyboard works after you have disconnected and reconnected it. This
  issue was first encountered in [#3149](https://github.com/canonical/mir/issues/3149).
  Please try this with multiple different `--console-provider=<PROVIDER>` arguments.
  To test:
    1. Have a keyboard that is plugged in
    2. Run `miral-shell` with a `--console-provider=<PROVIDER>`
    3. Unplug the keyboard
    4. Plug in the keyboard
    5. The keyboard should be usable
