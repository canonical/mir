---
discourse: 4911,5164,5603,6756,8037
---

# Write Your First Wayland Compositor
This tutorial will guide you through writing a simple compositor: the
installation of dependencies, building it, and running it. You'll be impressed
by how easy it is to create a Wayland compositor using Mir!

## Assumptions
This tutorial assumes that:
1. The reader is familiar with C++ and CMake.
2. The reader has cmake and a C++ compiler installed

## A barebones Mir compositor
This section will cover the needed dependencies and how to install them, the
minimum code needed for a Mir compositor, how to build this code, and finally
how to run your compositor.

### Dependencies
Before you start coding, you'll need to install `libmiral`, and
`mir-graphics-drivers-desktop` which can be done on different distros as
follows:

Installing dependencies on Debian and its derivatives:
```sh
sudo apt install libmiral-dev mir-graphics-drivers-desktop
```

<details>
<summary> Installing Dependencies on Other Distros </summary>

Installing dependencies on Fedora
```sh
sudo dnf install mir-devel libxkbcommon
```
Installing dependencies on Alpine
```sh
sudo apk add mir-dev
```
</details>


### Code
The following code block is the bare minimum you need to run a Mir compositor:
```cpp
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

This program creates a floating window manager with basic window management
capabilities such as controlling multiple windows, minimizing and maximizing,
and handling mouse input. This is done with the help of MirAL (Mir Abstraction
Layer) which gives you a high level interface to work with Mir.

### Building
To compile this simple program, you can use this `CMakeLists.txt` file:
```cmake
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

Then to build:
```sh
cmake -B build
cmake --build build
```

### Running
To run, you can run nested in an X or Wayland session, or from a virtual
terminal, just like the demo applications in [Learn What Mir Can
Do](learn-what-mir-can-do.md). For example, if we were to run inside an
existing Wayland session, we'd run the following command:
```sh
WAYLAND_DISPLAY=wayland-99 ./build/demo-mir-compositor
```
You'll see a window housing the compositor with a black void filling it. To
fill this void with some content, you can run the following from another
terminal:
```sh
WAYLAND_DISPLAY=wayland-99 bomber
```
You're free to replace `bomber` with any other Wayland compatible application.

## Next steps
Now that you have your base compositor working, feel free to check out these guides on how to further develop your compositor:

- [How to specify start up applications](how-to/how-to-specify-startup-apps.md)
- [How to handle user input](how-to/how-to-handle-keyboard-input.md)
- [Specifying CSD/SSD Preference](how-to/specifying-csd-ssd-preference.md)
