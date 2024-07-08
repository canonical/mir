---
discourse: 4911,5164,5603,6756,8037
---

# Tutorial
This tutorial will instruct you on how to get started building your first
Wayland compositor with Mir. Additionally, we will provide information on how
to install some demo compositors that will give you a sense of what you can
build with Mir.

## Installing Mir Libraries
The Mir libraries and demos are available on Ubuntu, Fedora, Debian and Arch.

### Installing Mir on Ubuntu
As a matter of policy Ubuntu does not routinely update packages after a series
is released. This means the Mir team cannot reasonably guarantee that all series
will have the latest Mir release available at all times.

The latest Mir release is available for all supported Ubuntu series from the
Mir team's "release PPA". To add the PPA to your system:

```sh
sudo add-apt-repository --update ppa:mir-team/release
```

Afterwards, you will be able to install any of the libraries that are published
by the project. The library that one might be interested in as a compositor author
is `libmiral`. To install it, you may run:


```sh
sudo apt install libmiral7 libmiral-dev mir-graphics-drivers-desktop
```

To remove the PPA from your system:

```sh
sudo ppa-purge mir-team/release
```

### Installing Mir on Fedora

On Fedora, Mir is available from the archive:

```sh
sudo dnf install mir-devel mir-server-libs
```

### Installing Mir on Debian
On Debian, Mir is available as `mir` (https://tracker.debian.org/pkg/mir):

```
sudo apt install mir
```


### Installing Mir on Arch

On Arch Linux, you can install the [mir](https://aur.archlinux.org/packages/mir/) package from the AUR.

## Installing and Running Demo Compositors
Mir provides a number of few example compositors in the
[examples folder](https://github.com/canonical/mir/tree/main/examples) of the Mir project.
Each folder in the "examples" folder is an independent Wayland compositor.

### Installing demos on Ubuntu

```sh
sudo apt install mir-demos mir-graphics-drivers-desktop
```

### Installing Mir demos manually
For Linux distributions that don't currently package Mir you need to build it
yourself. (See [Getting Involved in Mir](how-to/getting_involved_in_mir)).

### Running Mir demos
The main mir demo that you might be interested in is `miral-app`, which provides
access to a number of different window manager demos depending on which arguments
are provided to it. By default, it runs `miral-shell`, which is a floating window manager
with  pleasant startup screen. To run the demos, you can  simply run `miral-app`
from a terminal in your current session (e.g. a gnome-shell session):

```sh
miral-app
```

This will open up a new window in your compositor that contains a nested
"floating window manager" compositor.
Likewise, you can switch to a different VT (using `Ctrl + Alt + F<NUMBER>`). From
there, you may log in and run:

```sh
miral-app
```

In both cases, you should see a startup screen with information on how to use the
compositor. The compositor should behave like a typical floating window manager.

#### Options to `miral-app`
The `miral-app` script provides options for using an alternative shell
(e.g. `miral-kiosk` as used by the mir-kiosk snap) and an alternative terminal.

    -kiosk                      use miral-kiosk instead of miral-shell
    -terminal <terminal>        use <terminal> instead of '/usr/bin/miral-terminal'

The default for `-terminal` is a script that tries to identify the system terminal
emulator and launch that. But another terminal, or indeed any application, can be used.

For  example:

    miral-app -kiosk -terminal supertuxkart

There are some additional options (listed with "-h") but those are the important
ones.
