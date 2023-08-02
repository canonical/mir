# Welcome to Mir {#mainpage}

**Mir** is set of libraries for building Wayland based shells. Mir
simplifies the complexity that shell authors need to deal with: it
provides a stable, well tested and performant platform with touch,
mouse and tablet input, multi-display capability and secure
client-server communications.

Mir deals with the bringup and configuration of a broad array of
graphics and input hardware, abstracts hardware differences away
from shell authors (transparently dealing with hardware quirks) and
integrates with system components such as greeters.

Window management is integrated into Mir with useful default behaviour
and is extremely customisable by shell authors using a simple high-level
API.

## Resources
- The repository can be found [here](https://github.com/MirServer/mir)
- If you want to use the Mir snaps, see: 
  [Make a Secure Ubuntu Web Kiosk](https://mir-server.io/docs/make-a-secure-ubuntu-web-kiosk)
  [Miriway: the snap](https://discourse.ubuntu.com/t/miriway-the-snap/35821)
- If you want to try out the Mir demos on desktop, see: \ref getting_and_using_mir

- If you want to get involved in Mir development, see: \ref getting_involved_in_mir

## Using Mir for server development

Install the headers and libraries for using libmiral in development:

    sudo apt install libmiral-dev

A `miral.pc` file is provided for use with `pkg-config` or other tools. For
example:

    pkg-config --cflags miral

The server API is introduced here: \ref introducing_the_miral_api


Mir developers and discussion can also be found on the \#mir-server IRC channel on Freenode.
