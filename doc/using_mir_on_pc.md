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
build installed. If you are running Ubuntu saucy
(see \ref installing_prebuilt_on_pc), you should be good to go.

If you built Mir from source code (see \ref building_source_for_pc), you
need to ensure you are using the proper Mesa at runtime. You can do that by
installing the Mesa packages from saucy or by building the custom Mesa yourself
and ensuring it can be found by Mir, e.g., by using LD_LIBRARY_PATH.

Using Mir as system compositor with X
-------------------------------------

Note: for this to work you need to have Mir and all its dependencies (which
include lightdm, Mesa and the Xorg drivers). The easiest way is to run saucy.

If you have installed unity-system-compositor it will have created a file in
/etc/lightdm/lightdm.conf.d/10-unity-system-compositor.conf to run XMir. If you
have build from source, to run X sessions under Mir, with Mir acting as the
system compositor, create the file
/etc/lightdm/lightdm.conf.d/10-unity-system-compositor.conf to look to look like
this:

    [SeatDefaults]
    type=unity

Now restart lightdm:

    $ sudo restart lightdm

Or you may simply reboot.

In theory, you should now find yourself back in Ubuntu and not notice
anything different. You can verify you're in Mir several ways:

    $ ps aux | grep unity-system-compositor
    $ grep -i xmir /var/log/Xorg.0.log
    $ ls -l /var/log/lightdm/unity-system-compositor.log

If problems occur, have a look at the following log files:

    /var/log/lightdm/lightdm.log
    /var/log/lightdm/unity-system-compositor.log
    /var/log/lightdm/x-0.log
    /var/log/Xorg.0.log

You may not be able to get to a terminal if your video system has locked up. In
that case secure shell into to your machine and read / copy these logs. They are
overwritten on next boot.

In any case, if you wish to deactivate XMir upon boot, simply comment out
the type=unity line from
/etc/lightdm/lightdm.conf.d/10-unity-system-compositor.conf, like this:

    [SeatDefaults]
    #type=unity

If you cannot boot into a graphical display or terminal to disable XMir, then
enter recovery mode as described in https://wiki.ubuntu.com/RecoveryMode. From
there edit / move / remove
/etc/lightdm/lightdm.conf.d/10-unity-system-compositor.conf and then reboot.
To modify the filesystem you will need to enter the following command:

    # mount / -o remount,rw

Running Mir natively
--------------------

You can also run Mir natively. To do so, log in to VT1 (Ctrl+Alt+F1) _after_
you are already logged in to X.  If you do so before then you will not be
assigned adequate credentials to access the graphics hardware and will get
strange errors.

VT switching away from Mir will only work if Mir is run as root. In this case
we need to change the permissions to the Mir socket so that clients can
connect:

    $ sudo mir_demo_server
    <Ctrl+Alt+F2> - log in to VT 2
    $ sudo chmod 777 /tmp/mir_socket
    $ some-mir-client
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

