(exp-systemd-integration)=

# Integrating with Systemd user sessions

On modern Linux systems, an instance of systemd is provided to manage
services in the user session. While in the past it has been possible
to mostly ignore this instance, some system components are starting to
make direct use of systemd. This leads to problems if a compositor
does not properly integrate with the system.

## Session startup process

When a user logs into a graphical session via the display manager, the
display manager will set up a [PAM][PAM] session, and execute the
command from the compositor's `/usr/share/wayland-sessions` desktop
file within that session. When that command exits, the PAM session
will close and the display manager will present it's login prompt
again.

One of the modules in the PAM session is
[pam_systemd][pam_systemd]. Among other things, this will organise for
a user instance of systemd to be launched if one is not already
running. (as might be the case if the user has another login session
active).

The user instance of systemd will start the `default.target`, which is
enough to have it set up the D-Bus session bus and manage session
services, but is not enough for services like xdg-desktop-portal that
will only run if `graphical-session.target` has been
started. Organising for this target to be started involves some
additional work from the compositor.

## Systemd graphical session targets

Systemd provides some [standard targets][systemd.special] as
integration points where services can hook into the startup
process. These include two related to graphical user sessions:

- `graphical-session-pre.target`: for services that should run before
  the compositor is available. This might be to set up sockets or
  environment variables that should be visible for everything starting
  at a later phase.
- `graphical-session.target`: for services that should run after the
  compositor is available.

For proper integration, we want the following to occur:

1. `graphical-session-pre.target` and `graphical-session.target` to be
   started on login, with the compositor started in between.
1. Everything started after the compositor should see the environment
   variables necessary to talk to the compositor
   (i.e. `WAYLAND_DISPLAY` and `DISPLAY`).
1. The targets and compositor should all stop together.
1. The PAM session should remain active until the compositor stops.
1. The configuration should not interfere with any other desktop
   session that might be instaled on the system.

To complicate matters, these standard targets can not be started
directly: they can only be started as a dependency of some other
target or service.

Instead, it is necessary to define a new target representing the
compositor's user session, along with a unit to run the compositor
itself.

## Compositor systemd units

The compositor should provide at least the following units:

1. A `$compositor-session.target` target to represent the graphical
   session.
1. A `$compositor.service` unit that runs the compositor itself.
1. A `$compositor-session-shutdown.target` target to help shut down
   the session.

The session target should bind to the generic
`graphical-session.target`, and start the compositor:

```
[Unit]
...
BindsTo=graphical-session.target
Before=graphical-session.target

Requires=$compositor.service
```

The compositor unit should ensure that it starts before the session
target, and is stopped when the session stops, and ensure the generic
`graphical-session-pre.target` is sequenced before it:

```
[Unit]
...
Requisite=$compositor-session.target
Before=$compositor-session.target
PartOf=$compositor-session.target

BindsTo=graphical-session-pre.target
After=graphical-session-pre.target
```

It should also update the environment so other services can
communicate with it:

```
[Service]
...
# TODO: the compositor should do this itself so these have the correct values
ExecStartPost=-systemctl --user set-environment WAYLAND_DISPLAY=wayland-0 DISPLAY=:0
# Make sure services
ExecStopPost=-systemctl --user unset-environment WAYLAND_DISPLAY DISPLAY
```

Finally, it should ensure that the session is stopped if the
compositor crashes:

```
[Unit]
...
OnFailure=$compositor-session-shutdown.target
OnFailureJobMode=replace-irreversibly
CollectMode=inactive-or-failed
```

The shutdown target's job is to make sure the main session target is
no longer running. So it will need something like this:

```
[Unit]
...
# Make sure the generic graphical-session targets have stopped
Conflicts=graphical-session.target graphical-session-pre.target
After=graphical-session.target graphical-session-pre.target

# Make sure the compositor specific session target has stopped
Conflicts=$compositor-sesssion.target
After=$compositor-sesssion.target

# Don't hang around afterwards
StopWhenUnneeded=true
```

## Compsitor session startup script

The compositor will still need to provide a command to execute in its
`/usr/share/wayland-sessions` desktop file. But this script should be
relatively simple now: transfer the environment configured by the PAM
session to systemd, start the session target, and then wait for the
target to stop.

Below is a simple version that implements the above:

```
#!/bin/sh

# Used by other components to identify the desktop session
export XDG_CURRENT_DESKTOP=compositor_name

# Transfer the environment to systemd
systemctl --user import-environment

# Start the session target and wait for it to stop
exec systemctl --user start --wait compositor-session.target
```

To quit the desktop session, ask systemd to start the
`$compositor-session-shutdown.target` target, which will tear
everything else down and cause the startup script to complete.

PAM: https://github.com/linux-pam/linux-pam
pam_systemd: https://www.freedesktop.org/software/systemd/man/latest/pam_systemd.html
systemd.special: https://www.freedesktop.org/software/systemd/man/latest/systemd.special.html
