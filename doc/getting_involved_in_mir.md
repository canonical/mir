Getting Involved in Mir  {#getting_involved_in_mir}
=======================

Getting involved
----------------

The best place to ask questions and discuss about the Mir project is
the \#ubuntu-mir IRC channel on freenode.

The Mir project is hosted on Launchpad: https://launchpad.net/mir


Getting Mir source & dependencies
---------------------------------
###  On Ubuntu

These instructions assume that you’re using Ubuntu 16.04LTS or later.

You’ll need a few development tools installed:

    $ sudo apt install devscripts equivs bzr

With these installed you can checkout Mir and get the remaining dependencies:

    $ bzr branch lp:mir
    $ sudo mk-build-deps -i

### On Fedora

You’ll need some development tools and packages installed:

    $ sudo dnf install bzr cmake gcc-c++ boost-devel mesa-libEGL-devel \
    mesa-libGLES-devel glm-devel protobuf-lite-devel protobuf-compiler \
    capnproto-devel capnproto glog-devel gflags-devel systemd-devel \
    glib2-devel wayland-devel mesa-libgbm-devel libepoxy-devel nettle-devel \
    libinput-devel libxml++-devel libuuid-devel libxkbcommon-devel \
    freetype-devel lttng-ust-devel libatomic qterminal qt5-qtwayland \
    python3-pillow libevdev-devel umockdev-devel gtest-devel gmock-devel

With these installed you can checkout Mir:

    $ bzr branch lp:mir

Building Mir
------------

    $ mkdir mir/build
    $ cd  mir/build
    $ cmake ..
    $ make

This creates an example shell (miral-shell) in the bin directory. This can be
run directly:

    $ bin/miral-shell

With the default options this runs in a window on X (which is convenient for
development).

The miral-shell example is simple, don’t expect to see a sophisticated launcher
by default. You can start mir apps from the command-line. For example:

    $ bin/miral-run gnome-terminal

That’s right, a lot of standard GTK+ applications will “just work” (the GDK
toolkit has a Mir backend). Any that assume the existence of an X11 and bypass
the toolkit my making X11 protocol calls will have problems though.

To exit from miral-shell press Ctrl-Alt-BkSp.

You can install the Mir examples, headers and libraries you've built with:
  
    $ sudo make install

### Preparing a VM to run Mir

Especially if you want to debug the shell without locking your system this might be a helpful setup:

- \ref setup_kvm_for_mir
- \ref setup_vmware_for_mir

### Contributing to Mir

Currently, the Mir code activity is performed on a development branch:
lp:~mir-team/mir/development-branch

This development branch is promoted to the branch used for the ubuntu archive
and touch images. Please submit any merge proposals against the development
branch.

Please file bug reports at: https://bugs.launchpad.net/mir

The Mir development mailing list can be found at: https://lists.ubuntu.com/mailman/listinfo/Mir-devel

The Mir coding guidelines are [here](cppguide/index.html).

### Working on Mir code

 - \ref md_README  "Mir Read me"
 - \ref md_HACKING "Mir hacking guide"
 - \ref component_reports
 - \ref dso_versioning_guide
 - \ref abi_compatibility_tools
 - \ref performance_framework
 - \ref latency "Measuring visual latency"

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
