Using Mir on a PC {#using_mir_on_pc}
=================

Before you begin
----------------

Before you can use Mir you need to ensure you have the proper custom Mesa build
installed. If you installed Mir using the packages from mir-team staging PPA
(see \ref installing_prebuilt_on_pc), you should be good to go.

If you built Mir from source code (see \ref building_source_for_pc), you need
to ensure you are using the proper Mesa at runtime. You can do that by
installing the Mesa packages manually from the staging PPA, or by building the
custom Mesa yourself and ensuring it can be found by mir, e.g., by using
LD_LIBRARY_PATH.

Using Mir as system compositor with X
-------------------------------------

Note: for this to work you need to have Mir and all its dependencies (which
include lightdm, Mesa and the Xorg drivers). The easiest way is to install
pre-built packages from the staging PPA.

To run X sessions under Mir, with Mir acting as the system compositor, edit
your /etc/lightdm/lightdm.conf to look to look like this:

    [SeatDefaults]
    user-session=ubuntu
    greeter-session=unity-greeter
    type=mir

Now restart lightdm:

    $ sudo service lightdm restart

In theory, you should now find yourself back in Ubuntu and not notice
anything different. You can verify you're in mir several ways:

    $ ps aux | grep mir
    $ grep -i xmir /var/log/Xorg.0.log
    $ ls -l /var/log/lightdm/mir.log

Running Mir natively
--------------------

You can also run Mir natively. To do so, log in to VT1 (Ctrl+Alt+F1) _after_
you are already logged in to X.  If you do so before then you will not be
assigned adequate credentials to access the graphics hardware and will get
strange errors.

Note that you can switch back to X using Alt+F7. But it is very important to
remember NOT to switch once you have any mir binaries running. Doing so will
currently make X die (!).

Now we want to run the mir server and a client to render something. The trick
is that we need to make sure the mir server is easy to terminate before ever
switching back to X. To ensure this, the server needs to be in the foreground,
but starting before your client (in the background). To do this, you must:

    $ (sleep 3; some-mir-client) & mir ; kill $!

Wait 3 seconds and the client will start. You can kill it with Ctrl+C. REMEMBER
to kill the mir processes fully before attempting to switch back to X or your X
login will die.

In case you accidentally killed your X login and ended up with a failsafe
screen, you might find on subsequent reboots you can't log in to X at all any
more (it instantly and silently takes you back to the login screen).  The fix
for this is to log in to a VT and:

    $ rm .Xauthority
    $ sudo restart lightdm

Getting some example client applications
----------------------------------------

If you installed mir using the packages from the mir-team staging PPA, you can
get some example programs by installing the `libmirclient-demos` package:

    $ sudo apt-get install libmirclient-demos

If you are building from source you can find client applications in the bin/
subdirectory of the build directory.
