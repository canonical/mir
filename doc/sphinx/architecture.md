# Architecture
This document describes the high-level architecture of *Mir*.

## Audience
This document is intended to provide contributors to *Mir* an overview of *Mir*'s systems. It is *not* intended to guide compositor-implementers, unless they are specifically interested in the internals of *Mir*.

Click [here](./architecture_diagram.rst) to see a diagram of *Mir*'s systems.

## Projects
The *Mir* project is split into three subprojects, namely:

1. *mirserver*: the core implementation of *Mir*
2. *miral*: the library used by compositor developers; relies on *mirserver* to do the heavy-lifting
3. *miroil*: the library used by Lomiri developers; relies on *mirserver* to o the heavy-lifting

The following explains the role that each plays in a bit more detail.

### mirserver
- Provides the core *Mir* runtime, with the entry-point being `mir::Server`
- Consumed by both `miral` and `miroil`
- Directories:
  - `include/server`
  - `src/server`
  - `src/include/server`

### miral
- The "Mir Abstraction Layer" (aka `MirAL` or `miral`) is an API that makes Mir easy for compositor-implementers to work with. It provides the core window management concepts and an interface that is more ABI stable than Mir's internal `mirserver` API
- Directories:
  - `include/miral`
  - `src/miral`

### miroil
- A custom *Mir* library for the [Lomiri](https://lomiri.com/) folks 
- Like `miral`, this library is built upon `mirserver`
- Directories:
  - `include/miroil`
  - `src/miroil`

## Core concepts of `mirserver`
The `mirserver` library includes a number of concepts that are key to handling *Mir*'s runtime. These are:

- [Compositor](#compositor)
- [Platforms](#platforms)
- [Input](#input)
- [Shell](#shell)
- [Scene](#scene)
- [Frontend Wayland](#frontend-wayland)
- [Wayland Protocol Generation](#wayland-protocol-generation)
- [Frontend XWayland](#frontend-xwayland)
- [Graphics](#graphics)
- [RendererGL](#renderer-gl)
- [Report](#report)
- [Console](#console)

### Compositor
- The `Compositor` is responsible for collecting the set of renderables and sending them off to the outputs to be displayed
- These renderables come from the [scene](#scene)
- The final composited image is sent to the displays that are provided by the [platforms](#platforms)
- Directories:
  - `src/include/server/mir/compositor`
  - `src/server/compositor`

## Platforms
- A **platform** is an adaptors that allows `mirserver` to work across different graphics, input, and rendering stacks
- *Mir* decides which platforms to use at runtime
- There are three distinct types of platforms:
  - *Display platform*: determines what the compositor is rendering *to* (e.g gbm-kms, x11, wayland)
  - *Input platform*: determines how the compositor receives input (e.g. evdev, X11, wayland)
  - *Rendering platform*: determines how we render (e.g. renderer-egl-generic)

### Input
- Deals with everything input (e.g. keyboard presses, mouse clicks)
- Provides the rest of *Mir* with access to these events
- The input platform is provided by the [platform](#platforms)
- Directories
  - `src/include/server/mir/input`
  - `src/server/input`

### Shell
- The `Shell` is responsible for managing the surfaces that the compositor wants to show.
- It provides an interface to create, update, and remove these surfaces from the display.
- As an example, the [wayland frontend](#frontend-wayland) might ask the `Shell` to create a new surface for it.
- Directories:
  - `src/include/server/mir/shell`
  - `src/server/shell`

### Scene
- The `Scene` provides an interface for the `Compositor` to access the list of renderable items
  - These renderables are derived from the surfaces that were added to the [shell](#shell)
  - You can think of the `Scene` as you would think of a "scene graph" in a 3D game engine
- Unlike the `Shell`, the `Scene` knows a lot about the Z-order of the surfaces. For this reason, it is also responsible for things like locking the display or sending surfaces to the back of the Z-order.
- Directories:
  - `src/include/mir/server/scene`
  - `src/server/scene`

### Frontend Wayland
- The *Mir* implementation of the wayland protocol
- The `WaylandConnector` class is the glue between the compositor and the specific details of the wayland protocol.
- Directories:
  - `src/server/frontend_wayland`

### Wayland Protocol Generation
- A subproject used to generate C++ classes from wayland protocol XML files.
- Directories:
  - `src/wayland`

### Frontend XWayland
- The *Mir* implementation of the xwayland protocol
- The `XWaylandConnector` class is the glue between the compositor and the specific details of the xwayland protocol
- This protocol talks to the `xwayland` server, which is spawned as a subprocess
- Directories:
  - `src/server/frontend_xwayland`

### Graphics
- A graphics abstraction layer sitting between the compositor and the specific graphics platform that it is using to render
- Directories:
  - `src/include/mir/server/graphics`
  - `src/server/graphics`

### Renderer GL
- The only supported `Renderer` type
- This is used by the compositor to build the final image
- Future renderers will be found alongside it in the `src/renderers` directory
- Directories:
  - `src/renderers/gl`

### Report
- Default logging and reporting facilities
- Directories:
  - `src/server/report`

### Console
- Handles `logind` and virtual-terminal related tasks for the compositor
- Directories:
  - `src/server/console`