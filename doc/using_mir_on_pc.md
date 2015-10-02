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
LD_LIBRARY_PATH.

Running Mir natively
--------------------

To run Mir natively on a PC/desktop/laptop, log in to VT1 (Ctrl+Alt+F1) _after_
you are already logged in to X.  If you do so before then you will not be
assigned adequate credentials to access the graphics hardware and will get
strange errors.

VT switching away from Mir will only work if Mir is run as root. In this case
we need to change the permissions to the Mir socket so that clients can
connect:

    $ sudo mir_demo_server
    <Ctrl+Alt+F2> - log in to VT 2
    $ sudo chmod a+rw /tmp/mir_socket
    $ some-mir-client -m /tmp/mir_socket
    <Ctrl+Alt+F1> - switch back to Mir. Watch your friends be amazed!

In case you accidentally killed your X login and ended up with a failsafe
screen, you might find on subsequent reboots you can't log in to X at all any
more (it instantly and silently takes you back to the login screen).  The fix
for this is to log in to a VT and:

    $ rm .Xauthority
    $ sudo restart lightdm

Getting some example client applications
----------------------------------------

You can get some example programs by installing the `mir-demos` package:

    $ sudo apt-get install mir-demos

If you are building from source you can find client applications in the bin/
subdirectory of the build directory.

