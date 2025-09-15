# Mir command line options

# FIXME

### `--arw-file`

Make server Wayland socket readable and writeable by all users. For debugging purposes only.

### `--platform-display-libs arg`

Comma separated list of libraries to use for platform output support, e.g. mir:x11,mir:wayland. If not provided the libraries are autodetected.

### `--platform-rendering-libs arg`

Comma separated list of libraries to use for platform rendering support, e.g. mir:egl-generic. If not provided the libraries are autodetected.

### `--platform-input-lib arg`

Library to use for platform input support, e.g. mir:stub-input. If not provided this is autodetected.

### `--platform-path arg (=/usr/local/lib/mir/server-platform)`

Directory to look for platform libraries.

### `-i [ --enable-input ] arg (=1)`

Enable input.

### `--compositor-report arg (=off)`

Configure compositor reporting. [{off,log,lttng}]

### `--display-report arg (=off)`

Configure display reporting. [{off,log,lttng}]

### `--input-report arg (=off)`

Configure input reporting. [{off,log,lttng}]

### `--seat-report arg (=off)`

Configure seat reporting. [{off,log}]

### `--scene-report arg (=off)`

Configure scene reporting. [{off,log,lttng}]

### `--shared-library-prober-report arg (=log)`

Configure shared library prober reporting. [{log,off,lttng}]

### `--shell-report arg (=off)`

Configure shell reporting. [{off,log}]

### `--composite-delay arg (=0)`

Number of milliseconds to wait for new frames from clients before compositing. Higher values result in lower latency but risk causing frame skipping.

### `--enable-touchspots`

Enable visual feedback of touch events. Useful for screencasting.

### `--cursor arg (=auto)`

Cursor type:

- auto: use hardware if available, or fallback to software.
- null: cursor disabled.
- software: always use software cursor.

### `--enable-key-repeat arg (=1)`

Enable server generated key repeat.

### `--idle-timeout arg (=0)`

Number of seconds Mir will remain idle before turning off the display when the session is not locked, or 0 to keep display on forever.

### `--idle-timeout-when-locked arg (=0)`

Number of seconds Mir will remain idle before turning off the display when the session is locked, or 0 to keep the display on forever.

### `--on-fatal-error-except`

Throw an exception when a fatal error condition occurs. This replaces the default behaviour of dumping core and can make it easier to diagnose issues.

### `--debug`

Enable debugging information. Useful when developing Mir servers.

### `--console-provider arg (=auto)`

Method used to handle console-related tasks (device handling, VT switching, etc):

- logind: use logind.
- vt: use the Linux VT subsystem. Requires root.
- none: support no console-related tasks. Useful for nested platforms which do not need raw device access and which don't have a VT concept.
- auto: detect the appropriate provider.

### `--vt arg (=0)`

VT to run on or 0 to use current. Only used when --console-provider=vt.

### `--vt-switching arg (=1)`

Enable VT switching on Ctrl+Alt+F\*. Only used when --console-provider=vt|logind.

### `--env-hacks arg`

Colon separated list of environment variable settings.

### `--mouse-handedness arg`

Mouse laterality:

- right: left button is primary.
- left: right button is primary.

### `--mouse-cursor-acceleration arg`

Acceleration profile for mice and trackballs:

- none: no acceleration.
- adaptive: cursor is accelerated.

### `--mouse-cursor-acceleration-bias arg`

Pointer acceleration speed of mice. Must be within range of [-1.0, 1.0].

### `--mouse-scroll-speed arg`

Mouse scroll speed scaling factor. Use negative values for natural scrolling.

### `--mouse-horizontal-scroll-speed-override arg`

Mouse horizontal scroll speed scaling factor. Use negative values for natural scrolling.

### `--mouse-vertical-scroll-speed-override arg`

Mouse vertical scroll speed scaling factor. Use negative values for natural scrolling.

### `--touchpad-disable-while-typing arg`

Disable touchpad while typing on keyboard. [true, false]

### `--touchpad-disable-with-external-mouse arg`

Disable touchpad if an external pointer device is plugged in. [true, false]

### `--touchpad-tap-to-click arg`

Enable or disable tap-to-click on this device. If enabled 1, 2, and 3 finger taps are mapped to left, right, middle click events. [true, false]

### `--touchpad-cursor-acceleration arg`

Acceleration profile for touchpads:

- none: no acceleration.
- adaptive: cursor accelerates.

### `--touchpad-cursor-acceleration-bias arg`

Pointer acceleration speed scaling factor for touchpads. Must be within range of [-1.0, 1.0].

### `--touchpad-scroll-speed arg`

Touchpad scroll scaling factor. Use negative values for natural scrolling.

### `--touchpad-horizontal-scroll-speed-override arg`

Touchpad horizontal scroll scaling factor. Use negative values for natural scrolling.

### `--touchpad-vertical-scroll-speed-override arg`

Touchpad vertical scroll scaling factor. Use negative values for natural scrolling.

### `--touchpad-scroll-mode arg`

Scroll mode for touchpads. Generates scroll events when:

- edge: single finger moves on right or bottom edges of touchpad.
- two-finger: two fingers move horzontally or vertically.
- button-down: mouse button held down.

### `--touchpad-click-mode arg`

Click mode for touchpad. Left, middle and right button click events generated when:

- none: no events generated.
- area: single finger tap on left, middle or right area.
- clickfinger: one, two or three fingers present when touchpad pushed down.

### `--touchpad-middle-mouse-button-emulation arg`

Generate middle mouse button click from a simultaneous left and right button click.

### `--key-repeat-rate arg (=25)`

Number of milliseconds between generated key repeat events.

### `--key-repeat-delay arg (=600)`

Number of millisecond to hold down a key before generating repeat events.

### `--cursor-theme arg (=default:DMZ-White)`

Colon separated cursor theme list, e.g. default:DMZ-Black.

### `--wayland-extensions arg`

Colon separated list of Wayland extensions to enable. If used, default extensions will NOT be enabled unless specified. Default extensions:

- mir_shell_v1
- wl_shell
- wp_fractional_scale_manager_v1
- xdg_activation_v1
- xdg_wm_base
- zwp_text_input_manager_v1
- zwp_text_input_manager_v2
- zwp_text_input_manager_v3
- zxdg_decoration_manager_v1
- zxdg_output_manager_v1
- zxdg_shell_v6
  Additional supported extensions:
- ext_session_lock_manager_v1
- zwlr_foreign_toplevel_manager_v1
- zwlr_layer_shell_v1
- zwlr_screencopy_manager_v1
- zwlr_virtual_pointer_manager_v1
- zwp_idle_inhibit_manager_v1
- zwp_input_method_manager_v2
- zwp_input_method_v1
- zwp_input_panel_v1
- zwp_pointer_constraints_v1
- zwp_primary_selection_device_manager_v1
- zwp_relative_pointer_manager_v1
- zwp_virtual_keyboard_manager_v1

### `--add-wayland-extensions arg`

Colon separated list of Wayland extensions to enable in addition to default extensions. Use `all` to enable all supported extensions.

### `--drop-wayland-extensions arg`

Colon separated list of Wayland extensions to disable.

### `--enable-x11 arg (=0)`

Enable X11 support.

### `--x11-scale arg (=1.0)`

The scale to assume X11 apps use. Defaults to the value of GDK_SCALE or 1. Can be fractional. (Consider also setting GDK_SCALE in app-env-x11 when using this).

### `--xwayland-path arg (=/usr/bin/Xwayland)`

Path to Xwayland executable.

### `--x11-displayfd arg`

File descriptor to write X11 DISPLAY number to when ready to connect.

### `--focus-stealing-prevention arg (=0)`

Prevent newly opened windows from taking keyboard focus from an active window.

### `--window-manager arg (=floating)`

Window management strategy. [{floating|tiling}]

### `--window-management-trace`

Log trace message

### `--display-config arg (=sidebyside)`

Display configuration:

- clone: all screens show the same content.
- sidebyside: each screen placed to the right of the previous one.
- single: only the first screen used.
- static=filename: use layout specificed in \<filename>.

### `--translucent arg (=off)`

Select a display mode with alpha channel. [{on,off}]

### `--display-scale arg (=1.0)`

Pixel scale for all displays, e.g. 2.0.

### `--display-autoscale arg`

Automatically set pixel scale for displays so they have specified logical height in pixels, e.g. 1080.

### `--app-env arg (=GDK_BACKEND=wayland,x11:QT_QPA_PLATFORM=wayland:SDL_VIDEODRIVER=wayland:-QT_QPA_PLATFORMTHEME:NO_AT_BRIDGE=1:QT_ACCESSIBILITY:QT_LINUX_ACCESSIBILITY_ALWAYS_ON:MOZ_ENABLE_WAYLAND=1:_JAVA_AWT_WM_NONREPARENTING=1:-GTK_MODULES:-OOO_FORCE_DESKTOP:-GNOME_ACCESSIBILITY:-QT_IM_MODULE)`

Colon separated list of environment variables set when launching applications.

### `--app-env-amend arg`

Colon separated list of additional environment variables added to --app-env. Multiple additions may be specified by providing the argument multiple times.

### `--app-env-x11 arg (=GDK_BACKEND=x11:QT_QPA_PLATFORM=xcb:SDL_VIDEODRIVER=x11)`

Colon separated list of additional environment variables set when launching X11 applications. Adds to --app-env and --app-env-amend.

### `--keymap arg (=us,jp,cm+,,dvorak+grp_led:scroll)`

Keymap to use. Specified in the form \<layout>\[+\<variant>[+\<options>]\], e.g. "gb" or "cz+qwerty" or "de++compose:caps"

### `--startup-apps arg`

Colon separated list of startup apps.

### `--shell-wallpaper-font arg (=/usr/share/fonts/truetype/ubuntu/Ubuntu-B.ttf)`

Font file to use for wallpaper.

### `--shell-terminal-emulator arg (=miral-terminal)`

Terminal emulator to use.

### `--enable-mouse-keys arg`

Enable mousekeys (controlling the mouse with the numpad).

### `--mouse-keys-acceleration-constant-factor arg`

The base speed for mousekey pointer motion.

### `--mouse-keys-acceleration-linear-factor arg`

The linear speed increase for mousekey pointer motion.

### `--mouse-keys-acceleration-quadratic-factor arg`

The quadratic speed increase for mousekey pointer motion.

### `--mouse-keys-max-speed-x arg`

The maximum speed in pixels/second the mousekeys pointer can reach on the x axis. Pass zero to disable the speed limit.

### `--mouse-keys-max-speed-y arg`

The maximum speed in pixels/second the mousekeys pointer can reach on the y axis. Pass zero to disable the speed limit.

### `-h [ --help ]`

Show command line help.

### `--help-markdown`

Show command line help in markdown format.

### `-V [ --version ]`

Display Mir version and exit.

## Options for module mir:gbm-kms

### `--bypass arg (=0)`

Enable bypass optimization for fullscreen surfaces.

### `--driver-quirks arg`

Driver quirks to apply. May be specified multiple times; multiple quirks are combined.

## Options for module mir:wayland

### `--wayland-host arg`

Display name for host compositor, e.g. wayland-0.

### `--wayland-surface-app-id arg`

Application ID for the window containing the Mir output.

### `--wayland-surface-title arg`

Title of the window containing the Mir output.

## Options for module mir:atomic-kms

### `--bypass arg (=0)`

Enable bypass optimization for fullscreen surfaces.

### `--driver-quirks arg`

Driver quirks to apply. May be specified multiple times; multiple quirks are combined.

## Options for module mir:x11

### `--x11-output arg (=1280x1024)`

Colon separated list of outputs to use. Dimensions are in the form WIDTHxHEIGHT[^SCALE], e.g. 1920x1080:3840x2160^2.

### `--x11-window-title arg (=Mir on X)`

Title of the window containing the Mir output.

## Options for module mir:virtual

### `--virtual-output arg`

Colon separated list of outputs to use. Dimensions are in the form WIDTHxHEIGHT, e.g. 1920x1080:3840x2160.
