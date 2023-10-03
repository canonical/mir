Welcome to Mir developer documentation
======================================

Mir is a next generation display server targeted as a replacement for the X
window server system to unlock next-generation user experiences for devices
ranging from Linux desktop to mobile and IoT devices powered by Ubuntu.

 - If you want to use the Mir snaps, see:
   [Make a Secure Ubuntu Web Kiosk](https://mir-server.io/docs/make-a-secure-ubuntu-web-kiosk)

 - If you want to try out the Mir demos on desktop, see: [Getting and using Mir](getting_and_using_mir.md)

 - If you want to get involved in Mir development, see: [Getting involved in Mir](getting_involved_in_mir.md)

Using Mir for server development
--------------------------------

Install the headers and libraries for using libmiral in development:

    sudo apt install libmiral-dev

A `miral.pc` file is provided for use with `pkg-config` or other tools. For
example:

    pkg-config --cflags miral

The server API is introduced here: [Introducing the MirAL API](introducing_the_miral_api.md)

```{toctree}
:hidden:

getting_and_using_mir
getting_involved_in_mir
architecture
architecture_diagram
api/library_root
```
