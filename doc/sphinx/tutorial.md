---
discourse: 4911,5164,5603,6756,8037
---

# Tutorial

Mir is a library for building things, not an end-user product, but it does come
with some demos to illustrate the possibilities.

## Getting Mir demos

The Mir libraries and demos are available on Ubuntu, Fedora and Arch. It has
also been built and tested on Debian but, at the time or writing, is not in the
archive.

For Linux distributions that don't currently package Mir you need to build it
yourself. (See [Getting Involved in Mir](how-to/getting_involved_in_mir)).

## Getting Mir on Ubuntu

You can install the Mir examples along with the Mir graphics drivers as follows:

    sudo apt install mir-demos mir-graphics-drivers-desktop

It is also useful to install Wayland support for Qt and qterminal:

    sudo apt install qterminal qtwayland5

### Getting the latest Mir release on Ubuntu

As a matter of policy Ubuntu does not routinely update packages after a series
is released. This means the Mir team cannot reasonably guarantee that all series
will have the latest Mir release available at all times.

The latest Mir release is available for all supported Ubuntu series from the
Mir team's "release PPA". To add the PPA to your system:

    sudo add-apt-repository --update ppa:mir-team/release

To remove the PPA from your system:

    sudo ppa-purge mir-team/release

## Getting Mir on Fedora

On Fedora Mir is available from the archive:

    sudo dnf install mir-demos

It is also useful to install qterminal:

    sudo dnf install qterminal

## Getting Mir on Arch

On Arch Linux, you can install the [mir](https://aur.archlinux.org/packages/mir/) package from the AUR.

## Using Mir demos

For convenient testing under X11 there's a "miral-app" script that wraps the
commands used to start a server and then launches a terminal (as the current
user):

    miral-app

If you're using a desktop that supports X11 then you can run this in a terminal
window. In that case Mir will automatically select a "Mir-on-X11" backend and
run in a Window.

Alternatively, to run Mir "natively" you can run the same command in a Virtual
Terminal.

### Running applications on Mir

If you use the terminal launched by `miral-app` Wayland applications can be
started as usual:

    kate
    neverputt
    gedit

### Options when running the Mir example shell

#### Script Options

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

#### `miral-shell` Options

The script will pass everything on the command-line following the first thing it
doesn't understand to `miral-shell`. The options can be listed by
`miral-shell --help`. The following are likely to be of interest:

    --window-management-trace           log trace message

Probably the main use for `miral-shell` is to test window-management (either of
a client toolkit or of a server) and this logs all calls to and from the window
management policy. This option is supported directly in the MirAL library and
works for any MirAL based shell - even one you write yourself.

    --window-manager arg (=floating)   window management strategy
                                       [{floating|tiling|system-compositor}]

This allows an alternative "tiling" window manager to be selected. *Note:
`--window-manager` is only supported by `miral-shell` (not `miral-kiosk`).*

For  example:

    miral-app --window-manager tiling

These options can also be specified in a configuration file. For example:

    $ cat ~/.config/miral-shell.config
    keymap=gb
    window-manager=tiling
