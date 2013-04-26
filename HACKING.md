Mir hacking guide
=================

Getting Mir
-----------

If you're reading this file then you've probably solved this one. ;)

However, for completeness Mir is a project on LaunchPad (https://launchpad.net/mir)
to grab a copy use the command:

    $ bzr branch lp:mir


Getting dependencies
--------------------
To succesfully build Mir there are a few packages required:

    $ apt-get install devscripts equivs cmake
    $ mk-build-deps --install --tool "apt-get -y" --build-dep debian/control


Building Mir
-----------

Mir is built using cmake. There are other options, but here's one way to
build the system:

    $ mkdir build
    $ cd build
    $ cmake ..
    $ make -j 8
    $ ctest


Coding Mir
----------

There's a coding style guide in the guides subdirectory. To build it into an
html file:

    $ make guides

(Or if you're reading the web version [here](cppguide/index.html)).


Code structure
--------------

Code structure: include

The include subdirectory contains header files "published" by corresponding parts
of the system. For example, include/mir/option/option.h provides a system-wide interface
for accessing runtime options published by the options component.

In many cases, there will be interfaces defined that are used by the component
and implemented elsewhere. E.g. the compositor uses RenderView which is implemented
by the surfaces component.

Files under the include directory should contain minimal implementation detail: interfaces
should not expose platform or implementation technology types etc. (And as public methods
are normally implementations of interfaces they do not use these types.)


Code structure: src

This comprises the implementation of Mir. Header files for use within the component
should be put here. The only headers from the source tree that should be included are
ones from the current component (ones that do not require a path component).


Code structure: test

This contains unit, integration and acceptance tests written using gtest/gmock. Tests
largely depend upon the public interfaces of components - but tests of units within
a component will include headers from within the source tree.


Code structure: 3rd_party

Third party code imported into our source tree for use in Mir. We try not to change
anything to avoid maintaining a fork.


Error handling strategy
-----------------------

If a function cannot meet its post-conditions it throws an exception and meets
AT LEAST the basic exception safety guarantee. It is a good idea to document the
strong and no-throw guarantees. http://www.boost.org/community/exception_safety.html


Running Mir
-----------

This is a brief guide describing how to run the Mir binaries once you have
them built. You might think it's obvious but there are some important things
you need to know to get it working, and also to prevent your existing X server
from dying at the same time.

1. Make sure your hardware is supported. That means you're using a Mesa driver,
   of which only intel and radeon families are presently supported. If you're
   logged in to X then run this command to verify an appropriate DRI driver
   is active:
       sudo pmap `pidof X` | grep dri.so
   or
       lsmod | grep drm

2. Make sure your software is supported. Mir requires a custom build of the
   Mesa packages to work at all. At the time of writing, these were available
   from:
       https://launchpad.net/~mir-team/+archive/staging
   If you don't have the right Mesa, you will get nothing on the screen and/or
   strange errors later.

3. Build Mir as described at the top of this document.

4. Log in to VT1 (Ctrl+Alt+F1) _after_ you are already logged in to X. If you
   do so before then you will not be assigned adequate credentials to access
   the graphics hardware and will get strange errors.

5. Note that you can switch back to X using Alt+F7. But it is very important
   to remember NOT to switch once you have any mir binaries running. Doing
   so will make X die (!).

6. Switch back to VT1: Ctrl+Alt+F1

7. Now we want to run the mir server and a client to render something. The
   trick is that we need to make sure the mir server is easy to terminate
   before ever switching back to X. To ensure this, the server needs to be in
   the foreground, but starting before your client (in the background). To
   do this, you must:
       cd <mir_source_dir>/build/bin
       (sleep 5; ./mir_demo_client_accelerated) & ./mir ; kill $!

   Wait 5 seconds and the client will start. You can kill it with Ctrl+C or
   Alt+F2,Alt+F1,Ctrl+C. REMEMBER to kill the mir processes fully before
   attempting to switch back to X or your X login will die.

8. In case you accidentally killed your X login and ended up with a failsafe
   screen, you might find on subsequent reboots you can't log in to X at all
   any more (it instantly and silently takes you back to the login screen).
   The fix for this is to log in to a VT and:
       rm .Xauthority
       sudo restart lightdm


Documentation
-------------

There are design notes and an architecture diagram (.dia) in the design
subdirectory.


