(tutorial-getting-started)=
# Getting started with Mir

This tutorial will introduce you to the basic functionality of Mir by running a demo compositor. By the end of
the tutorial, you will install and run a demo compositor, learn how to use Mir in different environments,
and learn about features that Mir provides.

If you are unfamiliar with Mir, start with this tutorial. Afterward, proceed to the
[developer tutorial](write-your-first-wayland-compositor.md) where you will develop
your first compositor with Mir.

## Install
First, we will install the Mir demo compositors.

Mir demos are available on Debian derivatives, Fedora, and Alpine. For distros
that don't have pre-built binaries, examples can be built from source.

### On Debian and its derivatives

```sh
sudo apt install mir-demos mir-graphics-drivers-desktop
```

### On Fedora
```sh
sudo dnf install mir-demos
```

### On Alpine
```sh
sudo apk add mir-demos mir
```

### On Arch Linux (AUR)
```
https://aur.archlinux.org/packages/mir
```

## Running a Mir compositor nested in Wayland or X11
Next, we will run one of the demo compositors that we just installed, namely `miral-app`.
`miral-app` is a simple, standalone demo compositor that provides a floating window manager.

Within your current graphical session, run:

```sh
miral-app
```

You should see an X11 window on your desktop. This is a full Wayland compositor
running nested inside your current session.

## Running a Mir compositor natively
Now that we've run `miral-app` in a nested session, let's run it natively. There are two ways to do this:
1. Launch the compositor from a virtual terminal
2. Launch the compositor from a login screen

### Launching from a virtual terminal
Let's begin by launching the compositor from a virtual terminal.

To do this, you can "vt-switch" from your current graphical session to a virtual terminal.
This is typically done by pressing `CTRL+ALT+F\<Number\>` and logging in.

Once logged in, run:

```sh
miral-app
```

The compositor should appear across your outputs.

***Do note that while, like many other Wayland compositors, Mir supports Nvidia
graphics cards they can cause stability issues due to quirks on the driver side.***

### Launching from a login screen
Next, let's launch `miral-app` from the login screen. In either gdm or lightdm,
you will find a dropdown list that contains the list of available Wayland compositors.
Open this list and choose **Mir Shell**. Log in, and `miral-shell` will be running.

## Running Clients in `miral-app`
Whether you're running Mir nested in your current session, from a VT or from
the login screen, you will be able to run Wayland clients in your current
session.

### Wayland Applications
To start, let's open up a terminal using `CTRL+ALT+T`. This will open up a
Wayland terminal client.

### X11 Applications
Next, rerun `miral-app` with the following flags:

```sh
miral-app --enable-x11 true
```

Afterward, you may open an X11 client using `CTRL+ALT+X`. You should see an
xterm session open if it is available on your system.

At this point, you may experiment opening other applications as well.

### Shell Component Applications
In addition to traditional applications, Mir provides the facilities for you to
run applications which are actually shell components, such as backgrounds, bars,
panels, lockscreens and more.

Some examples of shell components that work well with Mir are: wofi, waybar,
synapse, sway-notification-center, swaybg, swaylock, xfce-appfinder, and
xfce-panel.

As an example, we will demonstrate running `waybar`.

First, install `waybar` on your system.

Next, rerun `miral-app` with the following:

```sh
miral-app --add-wayland-extensions all
```

Finally, run `waybar` from within your compositor session. You should see a bar
appear at the top of your compositor.

## Conclusion
This tutorial has provided you with a brief introduction in running Mir-based
compositors on your system and launching applications inside of them.

If you want to play around with a pre-made setup suitable for daily use, you
can check out [Miriway](https://github.com/Miriway/Miriway/) and
[Miracle](https://github.com/miracle-wm-org/miracle-wm), which are used daily
by Mir developers. A list of all known Mir-based compositors can be found at
[Developing A Wayland Compositor Using
Mir](../how-to/developing-a-wayland-compositor-using-mir.md)
