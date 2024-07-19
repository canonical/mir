## Using Mir Libraries to Build Your Own Wayland Compositor
At this point, you're probably taken away by Mir's capabilities and want to use
it and the supporting libraries to build your own Wayland compositor. This
section will guide you through the installation and basic usage of Mir.

### Installation
Mir is available on Debian derivatives, Fedora, and Alpine among others. For
distros that don't have prebuilt binaries, it can be built from source.

<details>
<summary> Installing Mir Libraries on Debian and its derivatives </summary>

```sh
sudo apt install mir-graphics-drivers-desktop
```
</details>

<details>
<summary> Installing Mir Libraries on Fedora </summary>

```sh
sudo dnf install mir
```
</details>

<details>
<summary> Installing Mir Libraries on Alpine </summary>

```sh
sudo apk add mir
```
</details>

### A barebones Mir compositor

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

You can se it's barely longer than the usual C++ "Hello, world!" example. This
code should give you a minimal compositor with basic window management
capabilities such as controlling multiple windows, minimizing and maximizing,
handling input, etc... This is done with the help of MirAL (Mir Abstraction
Layer) which gives you a high level interface to work with Mir!

To compile, you'll need to install `libmircore`, `libmiral`, and
`mir-graphics-drivers-desktop` which can be done on Ubuntu (and Debian
derivatives) with the following command:
```sh
sudo apt install libmircore-dev libmiral-dev mir-graphics-drivers-desktop
```

**Note**: `mir-graphics-drivers-desktop` is only needed for Debian based distros. 

To compile this simple program:
```sh
g++ main.cpp -I/usr/include/mircore -I/usr/include/miral -lmiral -o demo-mir-compositor
```

To run, you can run nested in an X or Wayland session, or from a virtual
terminal, just like the demo applications in [Explore What Mir Is Capable of
Using Mir Demos](explore-mir-using-demos.md). For example, if we were to run
inside an existing Wayland session, we'd run the following command:
```sh
WAYLAND_DISPLAY=wayland-99 ./demo-mir-compositor 
```
You'll see a window housing the compositor with a black void filling it. To
fill this void with some content, you can run the following from another
terminal:
```sh
WAYLAND_DISPLAY=wayland-99 bomber
```
You're free to replace `bomber` with any other Wayland compatible application.
