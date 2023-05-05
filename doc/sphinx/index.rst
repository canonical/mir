.. index:

Welcome to Mir developer documentation
======================================

This is the developer documentation for working with or working on the Mir codebase.

If you want documentation on using Mir, or Ubuntu Frame (which is built using Mir) please see: `Mir Documentation <https://mir-server.io/docs/>`_

* If you want to try out the Mir demos on desktop, see: :ref:`getting_and_using_mir`

* If you want to get involved in Mir development, see: :ref:`getting_involved_in_mir`

Using Mir for server development
--------------------------------

Install the headers and libraries for using libmiral in development:

    sudo apt install libmiral-dev

A `miral.pc` file is provided for use with `pkg-config` or other tools. For
example:

    pkg-config --cflags miral

The server API is introduced here: \ref introducing_the_miral_api

.. toctree::
   :maxdepth: 1
   :caption: Content:

   ../getting_and_using_mir.rst
   ../getting_involved_in_mir.rst
   api/library_root

Indices and tables
------------------

* :ref:`genindex`
* :ref:`search`
