(how-to-enable-screencasting)=

# How to enable screencasting

## Requirements

Required wayland extensions to capture the whole output:

- `zwlr_layer_shell_v1` (for the capture source chooser),
- `ext_image_copy_capture_manager_v1`
- `ext_output_image_capture_source_manager_v1`

If you want to capture individual windows, you will also need to enable:

- `ext_foreign_toplevel_list_v1`
- `ext_foreign_toplevel_image_capture_source_manager_v1`

Required packages:

- xdg-desktop-portal
- xdg-desktop-portal-wlr
  - at least 0.8.0 for output capture
  - at least 0.8.2 for top level window capture
- wmenu or slurp (as a source chooser)

## Setup steps

1. The file at `/usr/share/xdg-desktop-portal/<compositor-name>-portals.conf` should contain the following:

```ini
[preferred]
default=gtk
org.freedesktop.impl.portal.Screenshot=wlr
org.freedesktop.impl.portal.ScreenCast=wlr
```

where `<compositor-name>` is the same value as `XDG_CURRENT_DESKTOP` when your compositor is running. This tells `xdg-desktop-portal` to override the screencast and screenshot implementations by using the `wlr` implementation.

2. The file at `/etc/xdg/xdg-desktop-portal-wlr/config` should contain the following:

```ini
[screencast]
max_fps=30
chooser_type=dmenu
chooser_cmd="wmenu -p 'Select a source to share:' -l 10"
```

Where `chooser_type=dmenu` runs the given `chooser_cmd`. `chooser_cmd` in this case should return the name of the output that's going to be shared. In the above case, `wmenu` will display a menu of capture sources at the top of the screen.

As an alternative, `slurp` can be used as a simpler picker:

```ini
chooser_type=simple
chooser_cmd=slurp -f %o -or
```

This chooser shows a transparent white overlay to indicate that it's running. Clicking on an output will select it for the screencast. Note that `slurp` does not provide a way to choose top level windows.

3. The following script should run automatically when your compositor starts up. For testing purposes, you can run the script after you start your compositor.

```sh
export XDG_CURRENT_DESKTOP=my-mir-compositor                            # Refer to step 1

# DBus has its own separate environment. For the sake of hygiene, we only copy over `XDG_CURRENT_DESKTOP` and `WAYLAND_DISPLAY`
VARIABLES="XDG_CURRENT_DESKTOP WAYLAND_DISPLAY"
dbus-update-activation-environment --systemd $VARIABLES

# Run `xdg-desktop-portal`, using the configuration above, it will take care of starting `xdg-desktop-portal-wlr`
systemd-run --user /usr/libexec/xdg-desktop-portal --replace
```

4. Try recording your screen with OBS Studio, sharing it with Google Meet, or the [getDisplayMedia() WebRTC sample](https://webrtc.github.io/samples/src/content/getusermedia/getdisplaymedia/). Depending on which source chooser has been configured, you should see a menu at the top of the screen or a transparent white rectangle covering the screen.

## Notes

- To enable extensions with a mir based compositor, you can append `--add-wayland-extensions=<extension 1>:<extension 2>` to your launch flags. For the steps above, you'd use `--add-wayland-extensions=zwlr_layer_shell_v1:ext_image_copy_capture_manager_v1:ext_output_image_capture_source_manager_v1:ext_foreign_toplevel_list_v1:ext_foreign_toplevel_image_capture_source_manager_v1`
- At present, xdg-desktop-portal-wlr's support for screencasting top level windows is a bit buggy. As of version 0.8.2, resizing the source window can trigger a protocol error in the portal service, and stop the cast. This will hopefully be fixed in future releases (see [xdg-desktop-portal-wlr #340](https://github.com/emersion/xdg-desktop-portal-wlr/pull/340)).
- To operate correctly with xdg-desktop-portal >= 1.21, you will need to integrate with the systemd user session to the extent that `graphical-session.target` is started.
