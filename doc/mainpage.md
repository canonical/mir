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

    sudo apt install libmiral-dev

A `miral.pc` file is provided for use with `pkg-config` or other tools. For
example: 

    pkg-config --cflags miral

The server API is introduced here: \ref introducing_the_miral_api
