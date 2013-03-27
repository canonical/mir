Building the source for a PC {#building_source_for_pc}
============================

Getting mir
-----------

Mir is a project on Launchpad (https://launchpad.net/mir). To grab a copy use
the command:

    $ bzr branch lp:mir


Getting dependencies
--------------------

To succesfully build Mir there are a few packages required. The easiest way
to get them is to use the packaging build dependencies:

    $ apt-get install devscripts equivs cmake
    $ mk-build-deps --install --tool "apt-get -y" --build-dep debian/control


Building mir
------------

Mir is built using cmake. You first need to create the build directory and
configure the build:

    $ mkdir build
    $ cd build
    $ cmake .. (possibly passing configuration options to cmake)

There are many configuration options for the mir project. The default options
will work fine, but you may want to customize the build depending on your
needs. The best way to get an overview and set them is to use the cmake-gui
tool:

    $ cmake-gui ..

The next step is to build the source and run the tests:

    $ make (-j8)
    $ ctest

Installing mir
--------------

To install mir just use the normal make install command:

    $ make install

This will install the mir libraries, executable, example clients and header
files to the configured installation location (/usr/local by default). If you
install to a non-standard location, keep in mind that you will probably need to
properly set the PKG_CONFIG_PATH environment variable to allow other
applications to build against mir, and LD_LIBRARY_PATH to allow applications to
find the mir libraries at runtime.
