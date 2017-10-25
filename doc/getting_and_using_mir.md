Getting and Using Mir  {#getting_and_using_mir}
=====================

Getting Mir examples
--------------------

These instructions assume that you’re using Ubuntu 17.10 later. For
earlier releases of Ubuntu or other distributions see \ref getting_involved_in_mir.

You can install the Mir examples along with the Mir graphics drivers as follows:

    $ sudo apt install mir-demos qterminal
    $ sudo apt install mir-graphics-drivers-desktop qtubuntu-desktop

Using Mir examples
------------------

For convenient testing there's a "miral-app" script that wraps the commands used
to start a server and then launches a terminal (as the current user):

    $ miral-app

To run independently of X11 you need to grant access to the graphics hardware
(by running as root) and specify a VT to run in. There's a "miral-desktop"
script that wraps to start the server (as root) and then launch a terminal
(as the current user):

    $ miral-desktop
    
For more options see *Options when running the Mir example shell* below.
    
### Running applications on Mir

If you use the command-line launched by miral-app or miral-desktop native Mir
applications (which include native Mir clients and those that use SDL or the 
GTK+, Qt toolkits) can be started as usual:

    $ mir_demo_client_egltriangle
    $ gedit
    $ sudo apt install kate neverball 
    $ kate
    $ neverball

From outside the MirAL session GTK+, Qt and SDL applications can still be run
using the miral-run script:

    $ miral-run gedit
    $ miral-run 7kaa

### Running for X11 applications

If you want to run X11 applications that do not have native Mir support in the
toolkit they use then the answer is Xmir: an X11 server that runs on Mir. First
you need Xmir installed:

    $ sudo apt install xmir

Then once you have started a miral shell (as above) you can use miral-xrun to
run applications under Xmir:

    $ miral-xrun firefox

This automatically starts a Xmir X11 server on a new $DISPLAY for the
application to use. You can use miral-xrun both from a command-line outside the
miral-shell or, for example, from the terminal running in the shell.

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
