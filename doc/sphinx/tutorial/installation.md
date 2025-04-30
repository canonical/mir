# Installation
In this tutorial, you will learn how to install the libraries provided by the Mir project
so that you may begin writing your compositor.

While Mir ships with a number of libraries, compositor developers are only expected to
interact with two primary libraries.

- `libmiral` - short for "Mir Abstraction Layer". Provides a high level interface to interact with and customize Mir compositors.
- `mir-graphics-drivers-desktop` - provides drivers so Mir can talk with different graphics drivers

To install these libraries:

## Ubuntu/Debian
```sh
sudo add-apt-repository ppa:mir-team/release
sudo apt update
sudo apt install libmiral-dev mir-graphics-drivers-desktop
```

## Fedora
```sh
sudo dnf install mir-devel libxkbcommon
```

## Alpine
```sh
sudo apk add mir-dev
```

## Arch Linux (AUR)
```
https://aur.archlinux.org/packages/mir
```

# Additional Libraries
While it is recommended that compositor authors use only the `miral` and `mircore`
libraries, you may find yourself in a situation where you need to access
other libraries that the Mir project provides. In the mir-team PPA, we split these
up across a few different packages so that consumers only install what they need.
You may find further information about each of these libraries in the
[libraries](../explanation/libraries.md) document.

To install these additional libraries on Ubuntu/Debian:

```sh
# Ubuntu/Debian only
sudo apt install libmirserver-dev libmirserver-internal-dev libmirrenderer-dev \
    libmirplatform-dev libmiroil-dev libmircore-dev libmircommon-dev libmirwayland-dev
```

Other packagers such as Fedora, Alpine, and Arch will include these libraries alongside
`miral` and `mircore` in their main installation, so you will not need to install anything else.
