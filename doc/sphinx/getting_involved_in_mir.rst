.. _getting_involved_in_mir:

Getting Involved in Mir
=======================

Getting involved
----------------

The Mir project website is `<http://www.mirserver.io/>`_,
the code is `hosted on GitHub <https://github.com/MirServer>`_

For announcements and other discussions on Mir see: 
`Mir on community.ubuntu <https://community.ubuntu.com/c/mir>`_

Getting Mir source and dependencies
-----------------------------------

You can get the source with::

    git clone https://github.com/MirServer/mir.git
    cd mir

You may need to install git for the system you are working on.

You’ll also need a few development tools installed. The exact dependencies and
instructions vary across distros.

On Ubuntu
^^^^^^^^^
::

    sudo apt install devscripts equivs
    mk-build-deps -i -s sudo

On Fedora and Alpine
^^^^^^^^^^^^^^^^^^^^

As we build these distros in Mir's CI you can copy the instructions
from the corresponding files under `spread/build`.

Building Mir
------------
::

    mkdir build
    cd  build
    cmake ..
    make

This creates an example shell (miral-shell) in the bin directory. This can be
run directly::

    bin/miral-shell

With the default options this runs in a window on X (which is convenient for
development).

The miral-shell example is simple, don’t expect to see a sophisticated launcher
by default. You can start mir apps from the command-line. For example::

    bin/miral-run qterminal

To exit from miral-shell press Ctrl-Alt-BkSp.

You can install the Mir examples, headers and libraries you've built with::
  
    sudo make install

Contributing to Mir
-------------------

Please file bug reports at: `<https://github.com/MirServer/mir/issues>`_

The Mir coding guidelines are `here <cppguide/index.html>`_.

Working on Mir code
^^^^^^^^^^^^^^^^^^^

 - `Mir Read me <https://github.com/MirServer/README.md>`_
 - `Mir hacking guide <https://github.com/MirServer/HACKING.md>`_
 - :ref:`component_reports`
 - :ref:`dso_versioning_guide`
 - \ref performance_framework
 - \ref latency "Measuring visual latency"
