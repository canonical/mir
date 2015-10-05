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
