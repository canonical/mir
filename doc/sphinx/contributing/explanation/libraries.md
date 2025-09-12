(mir-libraries)=

# Libraries
The Mir project is a collection of C++ libraries for writing Wayland
compositors. This document describes what those libraries are and how they
depend on one another.

## Public Libraries
The following libraries are intended for published for public consumption:

- `miral`
- `mircommon`
- `mircore`
- `miroil`
- `mirplatform`
- `mirserver`
- `mirwayland`

When you build the project, these libraries are found in
`<build_directory>/lib`.

Additionally, Mir publishes libraries that add support for different platforms.
These can be found in `<build_directory>/lib/server-modules`. These libraries
are loaded at runtime by Mir and provide access to the underlying graphics and
input hardware of that platform. The libraries that Mir provides are:

- `graphics-atomic-kms`
- `graphics-gbm-kms`
- `graphics-eglstream-kms`
- `graphics-wayland`
- `graphics-dummy`
- `server-x11`
- `server-virtual`
- `renderer-egl-generic`
- `input-evdev`
- `input-stub`

## Dependency Graph
In the following diagram, each arrow denotes that the library at the start of
the arrow depends on the library at the end of the arrow.

**Diagram**: A flow chart depicting how the libraries of Mir relate to one another.

```{mermaid} libraries.mmd
```

There are a few things of note in this diagram:

- `miral` and `mircore` are intended to be the primary user-facing libraries
- `miroil` is a special compatibility layer for the Lomiri project is not
  intended for general consumption
- `mirserver` depends on the most libraries as it is the brains of the operation
- `mircommon` provide common functionalities for every other library in the
  system
- The input and graphics platforms (represented as `input_X` and `graphics_X` in
  the diagram) are dynamically loaded by `mirserver` when the engine starts up.
