How To Enable Screencasting
===========================

## Requirements
Required packages:
- xdg-desktop-portal (tested with 1.18.4-1ubuntu2.24.04.1)
- xdg-desktop-portal-wlr (v0.5.0, newer versions will not work)

Required wayland extensions:
- `zwlr_layer_shell_v1` for slurp,
- `zwlr_xdg_screencopy_manager_v1` for `xdg-desktop-portal-wlr`

## Setup Steps

1. The file at `/usr/share/xdg-desktop-portal/portals/<compositor-name>.portal` should contain the following:
```
[portal]
DBusName=org.freedesktop.impl.portal.desktop.wlr
Interfaces=org.freedesktop.impl.portal.Screenshot;org.freedesktop.impl.portal.ScreenCast;
UseIn=my-mir-compsitor
```
where `my-mir-compositor` is the same value as `XDG_CURRENT_DESKTOP` when your compositor is running. This tells `xdg-desktop-portal` to override the screencast and screenshot implementations by using the `wlr` implementation.

2. The file at `/etc/xdg/xdg-desktop-portal-wlr/config` should  contain the following:
```
[screencast]
max_fps=30
chooser_type=simple
chooser_cmd=slurp -f %o -or
```

Where `chooser_type=simple` runs the given `chooser_cmd`. `chooser_cmd` in this case should return the name of the output that's going to be shared. In the example above, `slurp -f %o -or` uses a transparent white overlay to indicate that its running, if you click on a specific output, then it will return the name of that output.

3. The following script should run automatically when your compositor starts up. For testing purposes, you can run the script after you start your compositor.
```
export XDG_CURRENT_DESKTOP=my-mir-compositor                            # Refer to step 1
export XDG_DESKTOP_PORTAL_DIR=/usr/share/xdg-desktop-portal/portals     # Tells `xdg-desktop-portal` where custom configurations are stored

# DBus has its own separate environment. For the sake of hygiene, we only copy over `XDG_CURRENT_DESKTOP` and `WAYLAND_DISPLAY`
VARIABLES="XDG_CURRENT_DESKTOP WAYLAND_DISPLAY"
dbus-update-activation-environment --systemd $VARIABLES

# Run `xdg-desktop-portal`, using the configuration above, it will take care of starting `xdg-desktop-portal-wlr`
systemd-run --user --setenv=XDG_DESKTOP_PORTAL_DIR=${XDG_DESKTOP_PORTAL_DIR} /usr/libexec/xdg-desktop-portal --replace
```

4. Try recording your screen with OBS, or sharing it with google meet. You should get a transparent white overlay to indicate that you're picking an output to share. Once you click on an output, screencasting should automatically work from there.

## Notes
- To enable extensions with a mir based compositor, you can append `--add-wayland-extensions=<extension 1>:<extension 2>` to your launch flags. For the steps above, you'd use `--add-wayland-extensions=zwlr_layer_shell_v1:zwlr_screencopy_manager_v1`
- At the moment, mir's implementation of screencasting only allows sharing whole outputs.
