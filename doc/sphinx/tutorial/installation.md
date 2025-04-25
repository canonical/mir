# Installation
The Mir project provides libraries that aid in the development of a compositor.
The primary libraries that you will interact with are:

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
While it is recommended that you stick to the `miral` and `mircore`
libraries, you may find yourself in a situation where you need to access
other libraries that the Mir project provides. Most packagers will
include these libraries in their default installation, but the mir-team
ppa splits them up. You may install them on Ubuntu/Debian like so:

```sh
# Ubuntu/Debian only
sudo apt install libmirserver-dev
sudo apt install libmirserver-internal-dev
sudo apt install libmirrenderer-dev
sudo apt install libmirplatform-dev
sudo apt install libmiroil-dev
sudo apt install libmircore-dev
sudo apt install libmircommon-dev
sudo apt install libmirwayland-dev
```