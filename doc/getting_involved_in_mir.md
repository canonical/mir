# Getting Involved in Mir  {#getting_involved_in_mir}

## Getting involved

The Mir project website is <http://www.mirserver.io/>, 
the code is [hosted on GitHub](https://github.com/MirServer) 

For announcements and other discussions on Mir see: 
[Mir on community.ubuntu](https://community.ubuntu.com/c/mir) 

For other questions and discussion about the Mir project: 
the \#mirserver IRC channel on freenode.


## Getting Mir source and dependencies
###  On Ubuntu

You’ll need a few development tools installed:

    sudo apt install devscripts equivs git

With these installed you can checkout Mir and get the remaining dependencies:

    git clone --recursive https://github.com/MirServer/mir.git
    cd mir
    mk-build-deps -i -s sudo

### On Fedora

You’ll need some development tools and packages installed:

    sudo dnf install git cmake make gcc-c++ boost-devel mesa-libEGL-devel \
    mesa-libGLES-devel glm-devel protobuf-lite-devel protobuf-compiler \
    capnproto-devel capnproto glog-devel gflags-devel systemd-devel \
    glib2-devel wayland-devel mesa-libgbm-devel libepoxy-devel nettle-devel \
    libinput-devel libxml++-devel libuuid-devel libxkbcommon-devel \
    freetype-devel lttng-ust-devel libatomic qterminal qt5-qtwayland \
    python3-pillow libevdev-devel umockdev-devel gtest-devel gmock-devel \
    libXcursor-devel yaml-cpp-devel egl-wayland-devel libdrm-devel wlcs-devel

With these installed you can checkout Mir:

    git clone --recursive https://github.com/MirServer/mir.git
    cd mir

Building Mir
------------

    mkdir build
    cd  build
    cmake ..
    make

This creates an example shell (miral-shell) in the bin directory. This can be
run directly:

    bin/miral-shell

With the default options this runs in a window on X (which is convenient for
development).

The miral-shell example is simple, don’t expect to see a sophisticated launcher
by default. You can start mir apps from the command-line. For example:

    bin/miral-run qterminal

To exit from miral-shell press Ctrl-Alt-BkSp.

You can install the Mir examples, headers and libraries you've built with:
  
    sudo make install

### Contributing to Mir

Please file bug reports at: https://github.com/MirServer/mir/issues

The Mir development mailing list can be found at: https://lists.ubuntu.com/mailman/listinfo/Mir-devel

The Mir coding guidelines are [here](cppguide/index.html).

### Working on Mir code

 - \ref md_README  "Mir Read me"
 - \ref md_HACKING "Mir hacking guide"
 - \ref component_reports
 - \ref dso_versioning_guide
 - \ref performance_framework
 - \ref latency "Measuring visual latency"
