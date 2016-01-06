Using Mir on a PC {#using_mir_on_pc}
=================

Before you begin
----------------

Make sure your hardware is supported. That means you're using a Mesa driver,
of which intel, radeon, and nouveau families are supported. If you're logged
in to X then run this command to verify an appropriate DRI driver is active:

    sudo pmap `pidof X` | grep dri.so

or

    lsmod | grep drm

Before you can use Mir you need to ensure you have the proper custom Mesa
build installed. If you are running Ubuntu 13.10 or later
(see \ref installing_prebuilt_on_pc), you should be good to go.

If you built Mir from source code (see \ref building_source_for_pc), you
need to ensure you are using the proper Mesa at runtime. You can do that by
installing the Mesa packages from Ubuntu 13.10 (or later) or by building the
custom Mesa yourself and ensuring it can be found by Mir, e.g., by using
`LD_LIBRARY_PATH`.

### Getting some example client applications

You can get some example programs by installing the `mir-demos` package:

    $ sudo apt-get install mir-demos

If you are building from source you can find client applications in the `bin/`
subdirectory of the build directory.

Running Mir
-----------

Mir can run run either natively on mesa-kms or as an X application.

### Running Mir on X

To run Mir as an X client start it from an X terminal:

    $ mir_demo_server --launch-client mir_demo_client_multiwin
    
You can start additional Mir clients, for example (in a new terminal):

    $ mir_demo_egltriangle
    
To exit from Mir:
 
    <Ctrl+Alt+BkSp>
    
Note: up to Mir 0.18 it is also necessary to specify `--platform-input-lib` when
starting the server:
  - for Mir-0.17 add: `--platform-input-lib server-mesa-x11.so.6`
  - for Mir-0.18 add: `--platform-input-lib server-mesa-x11.so.7`

### Running Mir natively

To run Mir natively on a PC/desktop/laptop:

    $ sudo DISPLAY= mir_demo_server --vt 1 --arw-file
    
This will switch you to a Mir session on VT1. Switch back to your X-based 
desktop:

    <Ctrl+Alt+F7>
    
In a new terminal:

    $ mir_demo_client_multiwin -m /tmp/mir_socket
    
Switch back to Mir.

    <Ctrl+Alt+F1>
    
Watch your friends be amazed!

To exit from Mir:

    <Ctrl+Alt+BkSp>

In case you accidentally killed your X login and ended up with a failsafe
screen, you might find on subsequent reboots you can't log in to X at all any
more (it instantly and silently takes you back to the login screen).  The fix
for this is to log in to a VT and:

    $ rm .Xauthority
    $ sudo restart lightdm
