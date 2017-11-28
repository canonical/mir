Getting and Using Mir  {#getting_and_using_mir}
=====================

If you are not running Ubuntu things may not work as described below. While some
applications are able to use Mir's Wayland support only Ubuntu carries "native"
Mir support for GTK+, SDL2 and Qt applications and the Xmir X11 server. 

Mir was initially developed on and for Ubuntu. It is also available on Fedora. 
We aim to make it available for other distributions, but currently you need to
build it yourself. (See \ref getting_involved_in_mir).

## Getting Mir examples on Ubuntu

On Ubuntu the latest Mir release is available from the Mir team's "release PPA".
We recommend always using the latest release, but if you don't wish to use the
PPA, there will be an earlier release of Mir in the Ubuntu archive, but some
things may not work as described here.

To add the PPA to your system:

    $ sudo add-apt-repository ppa:mir-team/release
    $ sudo apt update

To remove the PPA from your system:

    $ sudo ppa-purge mir-team/release

You can install the Mir examples along with the Mir graphics drivers as follows:

    $ sudo apt install mir-demos qterminal
    $ sudo apt install mir-graphics-drivers-desktop qtubuntu-desktop

## Getting Mir examples on Fedora

On Fedora Mir is available from the archive. We recommend using qterminal with
Mir as Qt currently works better with Mir's Wayland support than other options.   

You can install the Mir examples as follows:

    $ sudo dnf install mir-demos qterminal

Using Mir examples
------------------

For convenient testing under X11 there's a "miral-app" script that wraps the
commands used to start a server and then launches a terminal (as the current
user):

    $ miral-app

To run independently of X11 you need to grant access to the graphics hardware
(by running as root) and specify a VT to run in. There's a "miral-desktop"
script that wraps to start the server (as root) and then launch a terminal
(as the current user):

    $ miral-desktop
    
For more options see *Options when running the Mir example shell* below.
    
### Running applications on Mir

__On Ubuntu__, if you use the command-line launched by `miral-app` or 
`miral-desktop` native Mir applications (which include native Mir clients
and those that use SDL or the GTK+, Qt toolkits) can be started as usual:

    $ sudo apt install kate neverputt 
    $ mir_demo_client_chain_jumping_buffers
    $ kate
    $ neverputt
    $ gedit

__On Fedora__, we can use the Wayland backends for SDL or Qt:

    $ sudo dnf install kate neverball-neverputt
    $ mir_demo_client_chain_jumping_buffers
    $ kate
    $ neverputt

From outside the MirAL session applications can be run using the miral-run script:

    $ miral-run kate
    $ miral-run neverputt

### Running for X11 applications

If you want to run X11 applications that do not have native Mir support in the
toolkit they use then the answer is an X11 server that runs on Mir. That could
be either `Xmir` or `Xwayland`.

On Ubuntu:
 
    $ sudo apt install xmir xwayland; 

On Fedora: 

    $ sudo dnf install xorg-x11-server-Xwayland 

Then once you have started a miral shell (as above) you can use `miral-xrun` to
run applications under Xmir or Xwayland:

    $ miral-xrun firefox

This automatically starts a Xmir X11 server on a new $DISPLAY for the
application to use. You can use miral-xrun both from a command-line outside the
miral-shell or, for example, from the terminal running in the shell.

If you have both `Xmir` and `Xwayland` installed you can choose as follows:

    $ miral-xrun -Xmir firefox
    $ miral-xrun -Xwayland firefox

### Options when running the Mir example shell

#### Script Options

Both the "miral-app" and "miral-desktop" scripts provide options for using an
alternative example shell (miral-kiosk) and an alternative to gnome-terminal.

    -kiosk               use miral-kiosk instead of miral-shell
    -launcher <launcher> use <launcher> instead of qterminal

In addition miral-desktop has the option to set the VT that is used:

    -vt       <termid>   set the virtual terminal [4]

There are some additional options (listed with "-h") but those are the important
ones.

#### miral-shell Options

The scripts can also be used to pass options to Mir: they pass everything on
the command-line following the first thing they don't understand. These can be
listed by `miral-shell --help`. Most of these options are inherited from Mir,
but the following MirAL specific are likely to be of interest:

    --window-management-trace           log trace message

Probably the main use for MirAL is to test window-management (either of a
toolkit or of a server) and this logs all calls to and from the window 
management policy. This option is supported directly in the MirAL library and
works for any MirAL based shell - even one you write yourself.

    --keymap arg (=us)                  keymap <layout>[+<variant>[+<options>]]
                                        , e,g, "gb" or "cz+qwerty" or 
                                        "de++compose:caps"

For those of us not in the USA this is very useful. Both the -shell and -kiosk
examples support this option.

    --window-manager arg (=floating)   window management strategy 
                                       [{floating|tiling|system-compositor}]

Is only supported by miral-shell and its main use is to allow an alternative
"tiling" window manager to be selected.

These options can also be specified in a configuration file. For example:

    $ cat ~/.config/miral-shell.config 
    keymap=gb
    window-manager=tiling