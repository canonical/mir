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
- `mirserverlttng`
- `mirwayland`

When you build the project, these libraries are found in
`<build_directory>/lib`.

Additionally, Mir publishes libraries that add support for different platforms.
These can be found in `<build_directory>/lib/server-modules`. These libraries
are loaded at runtime by Mir and provide access to the underlying graphics and
input hardware of that platform. The libraries that Mir provides are:

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

```{mermaid} libraries.mmd
```

There are a few things of note in this diagram:

- `miral` and `miroil` are intended to be the primary user-facing libraries as
  nothing else depends on them
- `mirserver` depends on the most libraries as it is the brains of the operation
- `mircore` and `mircommon` provide common functionalities for every other
  library in the system
- The input and graphics platforms (represented as `input_X` and `graphics_X` in
  the diagram) are dynamically loaded by `mirserver` when the engine starts up33333

## Private Static Libraries
Mir builds a number of static libraries that are linked to internally by
`mirserver`. These libraries are:

- `mirrenderergl`
  - `mirgl`
- `mircompositor`
- `mirconsole`
- `mirfrontend-wayland`
- `mirfrontend-xwayland`
- `mirgraphics`
- `mirinput`
- `mirreport`
  - `mirnullreport`
  - `mirserverlttng`
  - `mirlogging`
- `mirscene`
- `mirshell`
