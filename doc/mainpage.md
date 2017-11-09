Welcome to Mir {#mainpage}
==============

Mir is a next generation display server targeted as a replacement for the X
window server system to unlock next-generation user experiences for devices
ranging from Linux desktop to mobile and IoT devices powered by Ubuntu.

 - If you want to use the Mir snaps, see: 
   [Run a kiosk snap on Ubuntu Core](https://developer.ubuntu.com/core/examples/snaps-on-mir)

 - If you want to try out the Mir demos on desktop, see: \ref getting_and_using_mir

 - If you want to get involved in Mir development, see: \ref getting_involved_in_mir

Using Mir for server development
--------------------------------

Install the headers and libraries for using libmiral in development:

    $ sudo apt install libmiral-dev

A `miral.pc` file is provided for use with `pkg-config` or other tools. For
example: 

    $ pkg-config --cflags miral

The server API is introduced here: \ref introducing_the_miral_api

Using Mir for client development
--------------------------------

This is usually something you don't need to do explicitly, it is normally 
handled by a GUI toolkit (or library).

A toolkit can run on Mir in three ways: using Wayland protocols, using the Mir 
client API or using X11 translation by the Xmir server.

 - Qt, has "wayland" and "mir" plugins that can be selected by setting
   the QT_QPA_PLATFORM environment variable to "wayland" or "ubuntumirclient" 
   respectively.

 - SDL can be built with "wayland" and "mir" support and these options selected
   by setting the SDL_VIDEODRIVER environment variable to "wayland" or "mir".
   
If want to use the Mir client library directly (e.g. you are working on "mir" 
support for a toolkit or library) Install the headers and libraries for using
libmirclient in development:

    $ sudo apt install libmirclient-dev

A `miral.pc` file is provided for use with `pkg-config` or other tools. For
example:

    $ pkg-config --cflags mirclient

The client API documentation is here: \ref mir_toolkit
