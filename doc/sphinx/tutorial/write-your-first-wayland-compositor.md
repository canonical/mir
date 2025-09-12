(write-your-first-wayland-compositor)=

# Write your first Wayland compositor

This tutorial will guide you through writing a basic Mir compositor. By the end
of it, you will create, build, and run a program with basic window management
capabilities such as controlling multiple windows, minimizing and maximizing,
and handling mouse input.

## Assumptions

This tutorial assumes that:

- You are familiar with C++ and CMake.
- You have `cmake` and a C++ compiler installed.

## Install dependencies

Let's start by installing the dependencies required by our compositor.

`````{tab-set}

````{tab-item} Debian or derivatives
:sync: debian

For Debian and its derivatives, we only need two small packages:

* `libmiral-dev` - short for "Mir Abstraction Layer". Provides a high level
  interface to interact with and customize Mir compositors.
* `mir-graphics-drivers-desktop` - provides drivers so Mir can talk with
  different graphics drivers

```{literalinclude} ../../../spread/build/sbuild/task.yaml
:language: bash
:start-after: [doc:first-compositor:debian-dependencies-install]
:end-before: [doc:first-compositor:debian-dependencies-install-end]
:dedent: 6
```
````

````{tab-item} Fedora
:sync: fedora

```sh
dnf install mir-devel libxkbcommon-devel
```
````

````{tab-item} Alpine
:sync: alpine

```sh
apk add mir-dev libxkbcommon-dev
```

`````

## Create the compositor

Now that we have the dependencies installed, we are ready to begin programming
the compositor.

First let's create a new folder called `demo-mir-compositor`:

```sh
mkdir demo-mir-compositor
cd demo-mir-compositor
```

Next, create `CMakeLists.txt` with the following content:

```{literalinclude} ./first-wayland-compositor/CMakeLists.txt
---
language: cmake
---
```

Next, create `main.cpp` with the following content:

```{literalinclude} ./first-wayland-compositor/main.cpp
---
language: cpp
---
```

`MirRunner` is a class from `libmiral` that acts as the "entry point" of your
compositor.

`MirRunner runner` creates a `runner` object that can be used to configure your
compositor. To run the compositor you call `runner.run_with(...)`, passing in
different components to customize the behavior of the compositor. In this
example, `run_with()` is passed a function `set_window_management_policy` that
applies a `MinimalWindowManager` policy to the compositor. The compositor is
therefore created with basic window management capabilities such as controlling
multiple windows, minimizing and maximizing, and handling mouse input.

The runner allows for even more customization: enabling onscreen keyboards,
screen capture, pointer confinement, and so on.

Finally, build the cmake project:

```{literalinclude} ../../../spread/build/sbuild/task.yaml
---
language: bash
start-after: [doc:first-compositor:build]
end-before: [doc:first-compositor:build-end]
dedent: 6
---
```

## Run the compositor

You can run a compositor nested in an X or Wayland session, or from a virtual
terminal, just like the demo applications in [Getting started with
Mir](getting-started-with-mir.md).

For development, it is very useful to run your compositor within an existing
Wayland session, so let's do that first. To do this, run:

```{literalinclude} ../../../spread/build/sbuild/task.yaml
---
language: bash
start-after: [doc:first-compositor:run]
end-before: [doc:first-compositor:run-end]
dedent: 8
---
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
