---
discourse: 4911,5164,5603,6756,8037
---

# Write your first Wayland compositor
This tutorial will guide you through writing a basic Mir compositor. By the end of it, you will create, build, and run a program with basic window management
capabilities such as controlling multiple windows, minimizing and maximizing, and handling mouse input. 

This section will cover the needed dependencies and how to install them, the
minimum code needed for a Mir compositor, how to build this code, and finally
how to run your compositor.

## Assumptions

This tutorial assumes that:

* You are familiar with C++ and CMake.
* You have `cmake` and a C++ compiler installed.

## Installing dependencies

The example program requires 
* `libmiral` - short for "Mir Abstraction Layer". Provides a high level interface to interact with and customize Mir compositors.
* `mir-graphics-drivers-desktop` - provides drivers so Mir can talk with different graphics drivers

Install dependencies on Debian or derivatives:
```sh
sudo apt install libmiral-dev mir-graphics-drivers-desktop
```

<details>
<summary> Installing Dependencies on Other Distros </summary>

Instal dependencies on Fedora:
```sh
sudo dnf install mir-devel libxkbcommon
```
Install dependencies on Alpine:
```sh
sudo apk add mir-dev
```
</details>


## Coding a Mir compositor

The following code defines a barebones Mir compositor:

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


`MirRunner` is a class from `libmiral` that acts as the "entry point" of your compositor.

`MirRunner runner` creates a `runner` object that can be used to configure your compositor.  To run the compositor you call `runner.run_with(...)`, passing in different components to customize the behavior of the compositor. In this example, `run_with()` is passed a function `set_window_management_policy` that applies a `MinimalWindowManager` policy to the compositor. The compositor is therefore created with basic window management capabilities such as controlling multiple windows, minimizing and maximizing, and handling mouse input. 

Through the `runner`, you can add different functionality to the composer: enabling onscreen keyboards, screen capture, pointer confinement, and so on. 

## Building a Mir composer

To compile this simple program, create a `CMakeLists.txt` file with the following content:

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

Then build it:
```sh
cmake -B build
cmake --build build
```

## Running a Mir composer
You can run a composer nested in an X or Wayland session, or from a virtual terminal, just like the demo applications in [Getting started with Mir](learn-what-mir-can-do.md). 

For example, to run inside an existing Wayland session:
```sh
WAYLAND_DISPLAY=wayland-99 ./build/demo-mir-compositor
```
An all-black window with the compositor will pop up.

To change the black background of the window and display some content instead, open another terminal and run:
```sh
WAYLAND_DISPLAY=wayland-99 bomber
```
Try moving it around the screen, maximizing and minimazing it. This functionality is provided by the `MinimalWindowManager` policy that you have added to your composer. You can replace `bomber` with any other Wayland-compatible application.

## Next steps
Now that you have your base compositor working, check out these guides on how to further develop your compositor:

- [How to specify start up applications](/how-to/how-to-specify-startup-apps.md)
- [How to handle user input](/how-to/how-to-handle-keyboard-input.md)
- [Specifying CSD/SSD Preference](/how-to/specifying-csd-ssd-preference.md)
