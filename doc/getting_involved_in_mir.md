# Getting Involved in Mir  {#getting_involved_in_mir}

## Getting involved

The Mir project website is <http://www.mirserver.io/>, 
the code is [hosted on GitHub](https://github.com/MirServer) 

For announcements and other discussions on Mir see: 
[Mir on community.ubuntu](https://community.ubuntu.com/c/mir) 

For other questions and discussion about the Mir project: 
the [\#mirserver](https://web.libera.chat/?channels=#mir-server) IRC channel on Libera.Chat.


## Getting Mir source and dependencies

You can get the source with:

    git clone https://github.com/MirServer/mir.git
    cd mir

You may need to install git for the system you are working on.

You’ll also need a few development tools installed. The exact dependencies and
instructions vary across distros.

###  On Ubuntu

    sudo apt install devscripts equivs
    mk-build-deps -i -s sudo

### On Fedora and Alpine

As we build these distros in Mir's CI you can copy the instructions
from the corresponding files under `spread/build`.

## Building

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

## Contributing to Mir

Please file bug reports at: https://github.com/MirServer/mir/issues

The Mir development mailing list can be found at: https://lists.ubuntu.com/mailman/listinfo/Mir-devel

The Mir coding guidelines are [here](https://mir-server.io/doc/cppguide/index.html).


## Working on code
 - Hacking guidelines can be found here: [Mir Hacking Guides](../HACKING.md)
 - Information about runtime tracing can be found here: [Component Reports](./component_reports.md)
 - A guide on versioning Mir DSOs: [DSO Versioning Guide](./dso_versioning_guide.md)
