---
discourse: 4911,5164,5603,6756,8037
---

# Write your first Wayland compositor
This tutorial will guide you through writing a basic Mir compositor. By the end
of it, you will create, build, and run a program with basic window management
capabilities such as controlling multiple windows, minimizing and maximizing,
and handling mouse input.

## Assumptions
This tutorial assumes that:

* You are familiar with C++ and CMake.
* You have `cmake` and a C++ compiler installed.

## Install Dependencies
Let's start by installing the dependencies required by our compositor.

`````{tab-set}

````{tab-item} Debian or derivatives
:sync: debian

For Debian and its derivatives, we only need two small packages:

* `libmiral-dev` - short for "Mir Abstraction Layer". Provides a high level
  interface to interact with and customize Mir compositors.
* `mir-graphics-drivers-desktop` - provides drivers so Mir can talk with
  different graphics drivers

```sh
sudo apt install libmiral-dev mir-graphics-drivers-desktop
```
````

````{tab-item} Fedora
:sync: fedora

```sh
sudo dnf install mir-devel libxkbcommon
```
````

````{tab-item} Alpine
:sync: alpine

```sh
sudo apk add mir-dev
```

`````

## Create the Compositor
Now that we have the dependencies installed, we are ready to begin programming
the compositor.

First let's create a new folder called `demo-mir-compositor`:

```sh
mkdir demo-mir-compositor
cd demo-mir-compositor
```

Next, create `CMakeLists.txt` with the following content:

```cmake
# CMakeLists.txt
cmake_minimum_required(VERSION 3.5)

project(demo-mir-compositor)

set(CMAKE_CXX_STANDARD 23)

include(FindPkgConfig)
pkg_check_modules(MIRAL miral REQUIRED)
pkg_check_modules(XKBCOMMON xkbcommon REQUIRED)

add_executable(demo-mir-compositor main.cpp)

target_include_directories(demo-mir-compositor PUBLIC SYSTEM ${MIRAL_INCLUDE_DIRS})
target_link_libraries(     demo-mir-compositor               ${MIRAL_LDFLAGS})
target_link_libraries(     demo-mir-compositor               ${XKBCOMMON_LIBRARIES})
```

Next, create `main.cpp` with the following content:

```cpp
/// main.cpp

#include <miral/runner.h>
#include <miral/minimal_window_manager.h>
#include <miral/set_window_management_policy.h>

using namespace miral;

int main(int argc, char const* argv[])
{
    MirRunner runner{argc, argv};

    return runner.run_with(
        {
            set_window_management_policy<MinimalWindowManager>()
        });
}
```


`MirRunner` is a class from `libmiral` that acts as the "entry point" of your
compositor.

`MirRunner runner` creates a `runner` object that can be used to configure your
compositor.  To run the compositor you call `runner.run_with(...)`, passing in
different components to customize the behavior of the compositor. In this
example, `run_with()` is passed a function `set_window_management_policy` that
applies a `MinimalWindowManager` policy to the compositor. The compositor is
therefore created with basic window management capabilities such as controlling
multiple windows, minimizing and maximizing, and handling mouse input.

The runner allows for even more customization: enabling onscreen keyboards,
screen capture, pointer confinement, and so on.


Finally, build the cmake project:

```sh
cmake -B build
cmake --build build
```

## Run the compositor
You can run a compositor nested in an X or Wayland session, or from a virtual
terminal, just like the demo applications in [Getting started with
Mir](getting-started-with-mir.md).

For development, it is very useful to run your compositor within an existing
Wayland session, so let's do that first. To do this, run:

```sh
WAYLAND_DISPLAY=wayland-99 ./build/demo-mir-compositor
```

An all-black window with the compositor will pop up.

Next, let's open up an application in our compositor, namely the `bomber` arcade
game.
From another terminal, run:

```sh
WAYLAND_DISPLAY=wayland-99 bomber
```

An window displaying the `bomber` application should appear in the compositor.
You may try moving the window around the screen, maximizing it or restoring it.

## Next steps
Now that you have your base compositor working, check out these guides on how to
further develop your compositor:

- [How to specify start up applications](/how-to/how-to-specify-startup-apps.md)
- [How to handle user input](/how-to/how-to-handle-keyboard-input.md)
- [Specifying CSD/SSD Preference](/how-to/specifying-csd-ssd-preference.md)
