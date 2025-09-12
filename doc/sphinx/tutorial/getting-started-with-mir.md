(tutorial-getting-started)=

# Getting started with Mir

This tutorial will introduce you to the basic functionality of Mir by running a demo compositor. By the end of
the tutorial, you will install and run a demo compositor, learn how to use Mir in different environments,
and learn about features that Mir provides.

## Install Mir Demos

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

## Run a Mir compositor nested in Wayland or X11

Next, we will run one of the demos that we just installed, namely `miral-app`.
`miral-app` is a a script that handles running a Mir shell with a basic GUI.

Within your current graphical session, run:

```sh
miral-app
```

You should see an X11 window on your desktop. This is a full Wayland compositor
running nested inside your current session.

## Run a Mir compositor natively

Now that we've run `miral-app` in a nested session, let's run it natively from a
virtual terminal.

To do this, you can "vt-switch" from your current graphical session to a virtual
terminal. This is typically done by pressing `CTRL+ALT+F\<Number\>` and logging
in.

Once logged in, run:

```sh
miral-app
```

The compositor should appear across your outputs.

Note: Mir supports NVIDIA graphics cards but they can cause stability issues due
to quirks on the driver side.

## Run Clients

Whether you're running Mir nested in your current session or from a virtual
terminal, you will be able to run Wayland clients in your current session.

### Run Wayland Applications

To start, let's open up a terminal using `CTRL+ALT+T`. This will open up a
Wayland terminal client.

### Run X11 Applications

Rerun `miral-app` with the following flags:

```sh
miral-app --enable-x11 true
```

Next, you may open an X11 client using `CTRL+ALT+X`. You should see an
xterm session open if it is available on your system.

At this point, you may experiment opening other applications as well.

### Run Shell Component Applications

In addition to traditional applications, Mir provides the facilities for you to
run applications which are actually shell components, such as backgrounds, bars,
panels and more.

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

## Next steps

Check out the [developer tutorial](write-your-first-wayland-compositor.md) to
build your first compositor with Mir.
