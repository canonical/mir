## Command line options

### `--arw-file`

Make socket filename globally rw (equivalent to chmod a=rw)

### `--platform-display-libs`

Libraries to use for platform output support (default: autodetect)

### `--platform-rendering-libs`

Libraries to use for platform rendering support (default: autodetect)

### `--platform-input-lib`

Library to use for platform input support (default: input-stub.so)

### `--platform-path`

Directory to look for platform libraries (default: /usr/local/lib/mir/server-platform)

### `-i [ --enable-input ]`

Enable input.

### `--compositor-report`

Compositor reporting [{log,lttng,off}]

### `--display-report`

How to handle the Display report. [{log,lttng,off}]

### `--input-report`

How to handle to Input report. [{log,lttng,off}]

### `--seat-report`

How to handle to Seat report. [{log,off}]

### `--scene-report`

How to handle the scene report. [{log,lttng,off}]

### `--shared-library-prober-report`

How to handle the SharedLibraryProber report. [{log,lttng,off}]

### `--shell-report`

How to handle the Shell report. [{log,off}]

### `--composite-delay`

Compositor frame delay in milliseconds (how long to wait for new frames from clients before compositing). Higher values result in lower latency but risk causing frame skipping. Default: A negative value means decide automatically.

### `--enable-touchspots`

Display visualization of touchspots (e.g. for screencasting).

### `--cursor`

Cursor (mouse pointer) to use [{auto,null,software}]

### `--enable-key-repeat`

Enable server generated key repeat

### `--idle-timeout`

Time (in seconds) Mir will remain idle before turning off the display when the session is not locked, or 0 to keep display on forever.

### `--idle-timeout-when-locked`

Time (in seconds) Mir will remain idle before turning off the display when the session is locked, or 0 to keep the display on forever.

### `--on-fatal-error-except`

On "fatal error" conditions [e.g. drivers behaving in unexpected ways] throw an exception (instead of a core dump)

### `--debug`

Enable extra development debugging. This is only interesting for people doing Mir server or client development.

### `--console-provider`

Console device handling
How Mir handles console-related tasks (device handling, VT switching, etc)
Options:
logind: use logind
vt: use the Linux VT subsystem. Requires root.
none: support no console-related tasks. Useful for nested platforms which do not need raw device access and which don't have a VT concept
auto: detect the appropriate provider.

### `--vt`

[requires --console-provider=vt] VT to run on or 0 to use current.

### `--vt-switching`

[requires --console-provider=(vt|logind)] Enable VT switching on Ctrl+Alt+F*.

### `--env-hacks`

Colon separated list of environment variable settings

### `--mouse-handedness`

Select mouse laterality: [right, left]

### `--mouse-cursor-acceleration`

Select acceleration profile for mice and trackballs [none, adaptive]

### `--mouse-cursor-acceleration-bias`

Set the pointer acceleration speed of mice within a range of [-1.0, 1.0]

### `--mouse-scroll-speed`

Scales mouse scroll. Use negative values for natural scrolling

### `--mouse-horizontal-scroll-speed-override`

Scales mouse horizontal scroll. Use negative values for natural scrolling

### `--mouse-vertical-scroll-speed-override`

Scales mouse vertical scroll. Use negative values for natural scrolling

### `--touchpad-disable-while-typing`

Disable touchpad while typing on keyboard configuration [true, false]

### `--touchpad-disable-with-external-mouse`

Disable touchpad if an external pointer device is plugged in [true, false]

### `--touchpad-tap-to-click`

Enable or disable tap-to-click on this device, with 1, 2, 3 finger tap mapping to left, right, middle click, respectively [true, false]

### `--touchpad-cursor-acceleration`

Select acceleration profile for touchpads [none, adaptive]

### `--touchpad-cursor-acceleration-bias`

Set the pointer acceleration speed of touchpads within a range of [-1.0, 1.0]

### `--touchpad-scroll-speed`

Scales touchpad scroll. Use negative values for natural scrolling

### `--touchpad-horizontal-scroll-speed-override`

Scales touchpad horizontal scroll. Use negative values for natural scrolling

### `--touchpad-vertical-scroll-speed-override`

Scales touchpad vertical scroll. Use negative values for natural scrolling

### `--touchpad-scroll-mode`

Select scroll mode for touchpads: [edge, two-finger, button-down]

### `--touchpad-click-mode`

Select click mode for touchpads: [none, area, clickfinger]

### `--touchpad-middle-mouse-button-emulation`

Converts a simultaneous left and right button click into a middle button

### `--key-repeat-rate`

The repeat rate for key presses

### `--key-repeat-delay`

The repeat delay for key presses

### `--display-config`

Display configuration [{clone,sidebyside,single,static=<filename>}]

### `--translucent`

Select a display mode with alpha[{on,off}]

### `--display-scale`

Scale for all displays

### `--display-autoscale`

Target height in logical pixels for all displays

### `--enable-x11`

Enable X11 support [{1|true|on|yes, 0|false|off|no}].

### `--x11-scale`

The scale to assume X11 apps use. Defaults to the value of GDK_SCALE or 1. Can be fractional. (Consider also setting GDK_SCALE in app-env-x11 when using this)

### `--xwayland-path`

Path to Xwayland executable

### `--x11-displayfd`

File descriptor to write X11 DISPLAY number to when ready to connect

### `--wayland-extensions`

Colon-separated list of Wayland extensions to enable. If used, default extensions will NOT be enabled unless specified. Default extensions:
  ext_session_lock_manager_v1
  mir_shell_v1
  wl_shell
  wp_fractional_scale_manager_v1
  xdg_activation_v1
  xdg_wm_base
  zwlr_foreign_toplevel_manager_v1
  zwlr_layer_shell_v1
  zwlr_screencopy_manager_v1
  zwlr_virtual_pointer_manager_v1
  zwp_idle_inhibit_manager_v1
  zwp_input_method_manager_v2
  zwp_input_method_v1
  zwp_input_panel_v1
  zwp_pointer_constraints_v1
  zwp_primary_selection_device_manager_v1
  zwp_relative_pointer_manager_v1
  zwp_text_input_manager_v1
  zwp_text_input_manager_v2
  zwp_text_input_manager_v3
  zwp_virtual_keyboard_manager_v1
  zxdg_decoration_manager_v1
  zxdg_output_manager_v1
  zxdg_shell_v6
Additional supported extensions:
  

### `--add-wayland-extensions`

Wayland extensions to enable in addition to default extensions. Use `all` to enable all supported extensions.

### `--drop-wayland-extensions`

Wayland extensions to disable.

### `--window-management-trace`

Log trace message

### `--timeout`

Seconds to run before exiting

### `--cursor-theme`

Colon separated cursor theme list (e.g. "DMZ-Black")

### `--print-input-events`

List input events on std::cout

### `--screen-rotation`

Rotate screen on Ctrl-Alt-<Arrow>

### `--test-client`

client executable

### `--test-timeout`

Seconds to run before sending SIGTERM to client

### `--cursor-scale`

Scales the mouse cursor visually. Accepts any value in the range [0, 100]

### `-h [ --help ]`

Show command line help

### `--help-markdown`

Show command line help in markdown format

### `-V [ --version ]`

Display Mir version and exit


