# Architecture
This document describes the high-level architecture of *Mir*, including a diagram and a brief introduction of the libraries.

## Audience
This document is intended to provide contributors to *Mir* an overview of *Mir*'s systems. It is *not* intended to guide compositor-implementers, unless they are specifically interested in the internals of *Mir*.

## Diagram
[Architecture Diagram](./architecture_diagram.rst)

## Top-Level Folders
### `/include`
- The `include` folder at the root of the project provides the public-facing API.
- This directory *strictly* includes the interfaces that are useful to a compositor-implementer.
- If one is only interested in implementing their own *Mir*-based compositor, they need not look any further than this directory.

### `/src`
- The `src` folder at the root of the project comprises the core implementation of *Mir*.

### `/examples`
- A collection of demo *Mir* compositors that are used as examples of how one might build a true compositor. Examples include:
  - `miral-app`
  - `miral-shell`
  - `mir_demo_server`

### `/tests`
- Tests for the project, including unit tests, integration tests, acceptance tests, and test framework.

## Libraries
The following is the library tree:
```
├── miral
├── miroil
├── mircore
├── mircommon
├── mircookie
└── mirserver
    ├── mircompositor
    ├── mirinput
    ├── mirfrontend
    ├── mirfrontend-wayland
    ├── mirfrontend-xwayland
    ├── mirshell
    ├── mirshelldecoration
    ├── mirscene
    ├── mirgraphics
    ├── mirreport
    ├── mirlttng
    ├── mirlogging
    ├── mirnullreport
    ├── mirconsole
    ├── mirgl
    └── mirrenderergl
├── mirplatform
└──  mirwayland
```

### miral
- The "Mir Abstraction Layer" (aka `MirAL` or `miral`) is an API that makes Mir easy for compositor-implementers to work with. It provides the core window management concepts and an interface that is more ABI stable than Mir's internal mirserver API.
- Public-facing
- Directories:
  - `include/miral`
  - `src/miral`
  
### miroil
- A custom *Mir* library for the [Lomiri](https://lomiri.com/) folks
- This is unimportant for 99% of people
- Directories:
  - `include/miroil`
  - `src/miroil`

### mircore
- Fundamental data structures like 2D points, Rectangles, File Descriptor, etc.
- Public-facing
- Directories:
  - `include/core`
  - `src/core`
  -
### mircommon
- Data structures and utility functions like Mir-event building, logging, keymaps, etc.
- Differs from `mircore` in that the concepts map strictly onto *Mir*'s concepts instead of being general concepts like a Rectangle
- Public-facing
- Directories:
  - `include/common`
  - `src/common`
  - `src/include/common`

### mircookie
- Provides event timestamps for inputs events that are difficult to spoof.
- Public-facing
- Directories:
  - `include/cookie`
  - `src/cookie`
  - `src/include/cookie`

### mirserver
- Provides the core *Mir* runtime, with the entry-point being the `mir::Server` class.
- The `Server` class has a `DisplayServer` which provides access to the interfaces that that support the server's runtime.
  - The `ServerConfiguration` provides a bunch of methods called `the_XYZ()` to access these various instances, where "XYZ" can be replaced with the interface of interest.
- The classes that the `Server` relies on for its runtime can be found organized in separate folders in this directory.
- Directories:
  - `include/server`
  - `src/server`
  - `src/include/server`
  
### mircompositor
- Included in `mirserver.so`
- The `Compositor` is responsible for collecting the set of renderables and sending them off to the outputs to be displayed.
- Directories:
  - `src/include/server/mir/compositor`
  - `src/server/compositor`

### mirinput
- Included in `mirserver.so`
- Deals with everything input (e.g. keyboard presses, mouse clicks)
- The `InputManager` provides access to the specific input platform that is running (e.g. X, Wayland, or libinput).
- The `InputDispatcher` is responsible for taking events bubbled up from the platform and sending them off to the appropriate listeners
- Directories
  - `src/include/server/mir/input`
  - `src/server/input`
  
### mirfrontend-wayland
- The *Mir* implementation of the wayland protocol.
- The `WaylandConnector` class is the glue between the compositor and the specific details of the wayland protocol.
- Directories:
  - `src/server/frontend_wayland`

### mirfrontend-xwayland
- The *Mir* implementation of the xwayland protocol.
- The `XWaylandConnector` class is the glue between the compositor and the specific details of the xwayland protocol.
- This protocol talks to the `xwayland` server, which is spawned as a subprocess.
- Directories:
  - `src/server/frontend_xwayland`

### mirshell
- The `Shell` is responsible for managing the surfaces that the compositor wants to show.
- It provides an interface to create, update, and remove these surfaces from the display.
- As an example, the `WaylandConnector` might ask the `Shell` to create a new surface for it.
- Directories:
  - `src/include/server/mir/shell`
  - `src/server/shell`
  
### mirshelldecoration
- Manages *Mir*'s server-side decoration of windows. Currently only used for X11 clients.
- Directories:
  - `src/server/shell/decoration`

### mirscene
- The `Scene` provides an interface for the `Compositor` to access the list of renderable items
  - These renderables are derived from the surfaces that were added to the `Shell`.
  - You can think of the `Scene` as you would think of a "scene graph" in a 3D game engine.
- Unlike the `Shell`, the `Scene` knows a lot about the Z-order of the surfaces. For this reason, it is also responsible for things like locking the display or sending surfaces to the back of the Z-order.
- Directories:
  - `src/include/mir/server/scene`
  - `src/server/scene`

### mirgraphics
- A graphics abstraction layer sitting between the compositor and the specific graphics platform that it is using to render.
- Directories:
  - `src/include/mir/server/graphics`
  - `src/server/graphics`
  
### mirreport
- Default logging and reporting facilities
- Directories:
  - `src/server/report`

### mirconsole
- Handles `logind` and virtual-terminal related tasks for the compositor
- Directories:
  - `src/server/console`

### mirgl
- A short list of helpers for GL things
- Directories:
  - `src/gl`
  
### mirrenderergl
- The only supported `Renderer` type for now
- Future renderers will be found alongside it in the `src/renderers` directory
- Directories:
  - `src/renderers/gl`
  
### mirplatform
- Graphics and input code that is shared between the platforms found in `src/platforms`
- Directories
  - `src/include/platform`
  - `src/platform`

### mirwayland
- A subproject used to generate C++ classes from wayland protocol XML files.
- Directories:
  - `src/wayland`

### platforms (the directory, not a namespace)
- A **platform** is a backend that the compositor selects at runtime
- There are three distinct platforms that the compositor makes use of:
  - *DisplayPlatform*: determines what the compositor is rendering *to* (e.g gbm-kms, x11, wayland)
  - *InputPlatform*: determines how the compositor receives input (e.g. evdev, X11, wayland)
  - *RenderingPlatform*: determines how we render (e.g. renderer-egl-generic)
- The platform is loaded from a `.so` file at runtime