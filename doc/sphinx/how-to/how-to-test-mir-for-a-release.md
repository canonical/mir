# How to test Mir for a release
The following document details the criteria that must be met before we can
publish an official release of Mir. When adding to this test plan, please
do not fail to consider how your additions may be automated in the future, as
this is the end goal of our release testing.

Note that all of these tests must be run _after_ a release branch is created
in the Mir repository, such that the right repositories are updated beforehand.

## Setup
1. Install mir from the release candidate PPA
    ```
    sudo add-apt-repository --update ppa:mir-team/rc
    sudo apt install mir-demos mir-platform-graphics-virtual mir-graphics-drivers-desktop mir-test-tools
    ```
2. Install the `mir-test-tools` from the `22/beta` _or_ `24/beta` track (depending
   on which you are trying to test):
    ```
    sudo snap refresh
    sudo snap install mir-test-tools --channel=24/beta
    # ... or ...
    sudo snap install mir-test-tools --channel=22/beta
    ```
3. Install `miriway` from the latest `beta` track:
    ```
    sudo snap refresh
    sudo snap install miriway --classic --chanel=latest/beta
    ```

## Tests
Manual tests must be performed across a combination of different display
platforms and different Ubuntu releases. It is important also that we test both
the `snap` and the `non-snap` version for each platform.

|                                | core24 snap | 24.04 LTS | 24.10 |
|--------------------------------|-------------|----------|------------|
| gbm-kms                        |             |          |            |
| eglstream-kms                  |             |          |            |
| eglstream-kms + gbm-kms hybrid |             |          |            |
| x11                        |             |          |            |
| wayland                    |             |          |            |
| virtual                        |             |          |            |

### Platform Selection
For the following tests, we must test that the compositor can start with the correct
display platform chosen. **Note that this section should NOT be run for each platform
by nature**. The remaining test sections _**should**_ be run for each platform
in the matrix.

1. Test that platforms are selected appropriately
   - When you are not on an Nvidia platform, verify that `gbm-kms` is selected
     for each output when the compositor is _not_ running in a hosted environment.
   - When you have an Nvidia card _and_ you are using Nvidia drivers, then
     the `eglstream-kms` platform should be selected for that particular output.
   - When you are hosted by an X compositor, the `x11` platform will be chosen

2. Platforms can be forced to run on a particular display platform if the environment supports it.
   Platforms are specified with `--platform-display-libs=mir:<platform_name>`
   - To specify `egl-stream-kms`, you must have an Nvidia card with proprietary drivers
   - You can force an Nvidia card to use `gbm-kms`
   - To specify `x11`, you must be on a hosted platform
   - To specify `wayland`, you must be on a hosted wayland platform

3. Check that the virtual platform can run and you can connect to it via a VNC:
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
    qtwayland5
   
    # Finally..
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

### Mir Configuration Options

1. Run any Mir compositor with different console providers and ensure that the compositor can start:
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

2. Run `miral-app` with different window managers and confirm that the window management
   styles work as expected:
    ```sh
    miral-app --window-manager=floating # traditional floating window manager
    miral-app --window-manager=tiling # a tiling window manager
    miral-app --kiosk # a typical kiosk
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