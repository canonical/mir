Building the source for a PC {#building_source_for_pc}
============================

Getting Mir
-----------

Mir is a project on Launchpad (https://launchpad.net/mir). To grab a copy use
the command:

    $ bzr branch lp:mir

The command above will download the latest development version of Mir into
the 'mir' directory (called the 'project directory' from now on).

Getting dependencies
--------------------

To succesfully build Mir there are a few packages required. The easiest way
to get them is to use the packaging build dependencies:

    $ sudo apt-get install devscripts equivs cmake

Then, in the project directory:

    $ sudo mk-build-deps --install --tool "apt-get -y" --build-dep debian/control


Building Mir
------------

Mir is built using CMake. You first need to create the build directory and
configure the build. In the project directory do:

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


Running Mir
-----------

The binaries created in the bin subdirectory of the project directory can be
used directly. For example,

    $ bin/mir_demo_server --launch_client bin/mir_demo_client_multiwin

Other examples described elsewhere in this documentation assume you're using the
installed version and simply need "bin/" adding to specify the local build.  


Install Mir
-----------

*It should not be necessary to install Mir for experimental purposes (see 
"Running Mir" above).* Further, if you are using an Ubuntu derived disto then
there's likely to be existing Mir binaries elsewhere that may interact badly 
with a second install.

To install Mir just use the normal make install command:

    $ sudo make install

This will install the Mir libraries, executable, example clients and header
files to the configured installation location (/usr/local by default).

NB You may need "sudo ldconfig" to refresh the cache before the installed
programs work.

If you install to a non-standard location, keep in mind that you will probably 
need to properly set the PKG_CONFIG_PATH environment variable to allow other
applications to build against Mir, and LD_LIBRARY_PATH to allow applications to
find the Mir libraries at runtime.


Building Mesa
-------------

*The Mesa packages shipped with Ubuntu are already built with the relevant Mir patches
and should work out of the box with Mir.*

For GL accelerated clients to use Mir they need to use a patched version of Mesa
that supports Mir.

The patch is hosted on GitHub:

    $ git clone https://github.com/RAOF/mesa.git

Compile as per normal instructions and pass --with-egl-platforms="mir,drm" to
the configure options. You will need libmirclient installed as shown above.
