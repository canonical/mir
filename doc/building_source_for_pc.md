Building the source for a PC {#building_source_for_pc}
============================

Getting Mir
-----------

Mir is a project on Launchpad (https://launchpad.net/mir). To grab a copy use
the command:

    $ bzr branch lp:mir


Getting dependencies
--------------------

To succesfully build Mir there are a few packages required. The easiest way
to get them is to use the packaging build dependencies:

    $ sudo apt-get install devscripts equivs cmake
    $ sudo mk-build-deps --install --tool "apt-get -y" --build-dep debian/control


Building Mir
------------

Mir is built using CMake. You first need to create the build directory and
configure the build:

    $ mkdir build
    $ cd build
    $ cmake .. (possibly passing configuration options to CMake)

There are many configuration options for the Mir project. The default options
will work fine, but you may want to customize the build depending on your
needs. The best way to get an overview and set them is to use the cmake-gui
tool:

    $ cmake-gui ..

The next step is to build the source and run the tests:

    $ make (-j8)
    $ ctest

To install Mir just use the normal make install command:

    $ make install

This will install the Mir libraries, executable, example clients and header
files to the configured installation location (/usr/local by default). If you
install to a non-standard location, keep in mind that you will probably need to
properly set the PKG_CONFIG_PATH environment variable to allow other
applications to build against Mir, and LD_LIBRARY_PATH to allow applications to
find the Mir libraries at runtime.

Building Mesa
-------------

For GL accelerated clients to use Mir they need to use a patched version of Mesa
that supports Mir.

The patch is hosted on GitHub:

    $ git clone https://github.com/RAOF/mesa.git

Compile as per normal instructions and pass --with-egl-platforms="mir,drm" to
the configure options. You will need libmirclient installed as shown above.

Building X.Org
--------------

To run an X server inside Mir you need to build a patched version of the X.Org
X server.

The patch is hosted on GitHub:

    $ git clone https://github.com/RAOF/xserver.git

Compile as per normal instructions and pass --enable-xmir to the configure
options. You will need libmirclient installed as shown above.

Building the X.Org drivers
--------------------------

To run an X server inside Mir you also need a patched version of your X.Org
video driver.

The three drivers - Intel, Radeon, and Nouveau -  are available on Launchpad:

    $ bzr branch lp:~mir-team/mir/xf86-video-intel-vladmir
    $ bzr branch lp:~mir-team/mir/xf86-video-ati-vladmir
    $ bzr branch lp:~mir-team/mir/xf86-video-nouveau

Compile as per normal instructions. These need to be built after the X server,
as they depend on new interfaces there.

Building Unity System Compositor
--------------------------------

If you want to run a full system using XMir then you need to use a system
compositor. For Ubuntu we have a system compositor project on Launchpad
(https://launchpad.net/unity-system-compositor). Compile with the following:

    $ bzr branch lp:unity-system-compositor
    $ cd unity-system-compositor
    $ mkdir build
    $ cd build
    $ cmake ..

You will need libmirserver installed as shown above.

To use the system compositor you need a display manager that supports it. At the
time of writing LightDM is the only display manager with support and has been
available since version 1.7.4.
