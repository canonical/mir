# Getting Involved in Mir

## Getting involved

The Mir project website is <https://mir-server.io/>,
the code is [hosted on GitHub](https://github.com/canonical/mir)

For announcements and other discussions on Mir see:
[Mir on discourse.ubuntu](https://discourse.ubuntu.com/c/mir/15)

For other questions and discussion about the Mir project, feel free to join the
[Matrix channel](https://matrix.to/#/#mir-server:matrix.org).


## Getting Mir source and dependencies

You can get the source with:

    git clone https://github.com/canonical/mir.git
    cd mir

You may need to install git for the system you are working on.

You’ll also need a few development tools installed. The exact dependencies and
instructions vary across distros.

###  On Ubuntu

    sudo apt-get build-dep ./

### On Fedora and Alpine

As we build these distros in Mir's CI you can copy the instructions
from the corresponding files under `spread/build`.

## Building

    cmake -S . -Bbuild
    cd build
    cmake --build .

This creates an example shell (miral-shell) in the bin directory. This can be
run directly:

    bin/miral-app

With the default options this runs in a window on X (which is convenient for
development).

The miral-shell example is simple, don’t expect to see a sophisticated launcher
by default. Within this window you can start a terminal with Ctrl-Alt-Shift-T
(the "Shift" is needed to avoid Ubuntu's DE intercepting the input). From this
terminal you can start apps. For example:

    $ gedit

To exit from miral-shell press Ctrl-Alt-Backspace.

You can install the Mir examples, headers and libraries you've built with:

    sudo cmake --build . --target install

## Contributing to Mir

Please file bug reports at: [https://github.com/canonical/mir/issues](https://github.com/canonical/mir/issues).

The Mir Discourse category can be found at: [https://discourse.ubuntu.com/c/mir/15](https://discourse.ubuntu.com/c/mir/15).

```{raw} html
The Mir coding guidelines are <a href=../../_static/cppguide/>here</a>.
```

## Working on code
 - Hacking guidelines can be found here: [Mir Hacking Guides](https://github.com/canonical/mir/blob/main/HACKING.md)
 - You can configure *Mir* to provide runtime information helpful for debugging
   by enabling component reports: [Component Reports](../explanation/component_reports.md)
 - A guide on versioning Mir DSOs: [DSO Versioning Guide](../reference/dso_versioning_guide.md)
