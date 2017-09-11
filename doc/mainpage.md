Welcome to Mir {#mainpage}
==============

Mir is a next generation display server targeted as a replacement for the X
window server system to unlock next-generation user experiences for devices
ranging from Linux desktop to mobile and IoT devices powered by Ubuntu.

 - If you just want to try out the Mir demos, see: \ref getting_and_using_mir

 - If you want to get involved in Mir development, see: \ref getting_involved_in_mir

Using Mir for client development
--------------------------------

Install the headers and libraries for using libmirclient in development:

    $ sudo apt install libmirclient-dev

A `miral.pc` file is provided for use with `pkg-config` or other tools. For
example:

    $ pkg-config --cflags mirclient

The client API documentation is here: \ref mir_toolkit

Using Mir for server development
--------------------------------

Install the headers and libraries for using libmiral in development:

    $ sudo apt install libmiral-dev

A `miral.pc` file is provided for use with `pkg-config` or other tools. For
example: 

    $ pkg-config --cflags miral

The server API is introduced here: \ref introducing_the_miral_api

The Mir Documentation
---------------------

The Mir documentation can be installed and read like this:

    $ sudo apt install mir-doc
    $ xdg-open /usr/share/doc/mir-doc/html/index.html

More detailed information about the motivation, scope, and high-level design
of Mir can be found at http://wiki.ubuntu.com/MirSpec .

