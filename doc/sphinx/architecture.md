# Overview
This document describes the high-level architecture of *Mir*, including a diagram and a brief introduction of the directory structure.

# Concepts
## MirAL
- The "Mir Abstraction Layer" (aka `mirAL` or `miral`) is a layer that makes the core Mir implementation easier for compositor-implementers to work with.
- If one wishes to implement a compositor themselves, they will spend most of their time interacting with the `mirAL` API and only really touch the `mir` API when they require core concepts like a `mir::geometry::Rectangle`.

## Mir
- The core implementation of the compositor that `MirAL` uses under the hood
- Responsible for doing the heavy-lifting of the compositor, such as:
  - Output detection and configuration
  - Compositing the image onto outputs
  - Implementing the [Wayland protocol](https://wayland.app/protocols/)
  - Providing the different platforms that can be used for both display and rendering
  - Handling user input and bubbling it up to `MirAL`
  - & more!

# Diagram
TODO: Implement the diagram

# Code Structure
This section will provide an overview of how the *Mir* codebase is organized through an exploration of  the project's directory structure.

## `/include`
- The `include` folder at the root of the project provides the public-facing API.
- This directory *strictly* includes the interfaces that are useful to a compositor-implementer.
- If one is only interested in implementing their own *Mir*-based compositor, they need not look any further than this directory.

## `/src`
- The `src` folder at the root of the project comprises the core implementation of *Mir*.

## `/src/miral`
- See [MirAL](#miral) for more details
- The public-facing interface for this implementation is in `/include/miral/miral`
- Most of the classes in this directory look like this:

    ```c++
    class MyClass
    {
    public:
        void operator()(mir::Server& server) const;
        ...
    };
    ```
  Instances of such a class (e.g. `X11Support`) are passed to the `MirRunner::run_with` function, wherein the overloaded operator function is called with the server as a parameter.

## `/src/server`
- Provides the core *Mir* runtime, with the entry-point being the `mir::Server` class.
- The `Server` class has a `DisplayServer` which contains the class instances that support the server's runtime.
  - The `ServerConfiguration` provides a bunch of methods called `the_XYZ()` to access these various instances, where "XYZ" can be replaced with the class of interest.
- The classes that the `Server` relies on for its runtime can be found organized in separate folders in this directory.

## `/src/server/compositor`
- The `Compositor` is responsible for collecting the set of renderables and sending them off to the outputs to be displayed.

## `/src/server/input`
- This folder deals with everything input (e.g. keyboard presses, mouse clicks)
- The `InputManager` provides access to the specific input platform that is running (e.g. X, Wayland, or libinput).
- The `InputDispatcher` is responsible for taking events bubbled up from the platform and sending them off to the appropriate listeners

## `/src/server/frontend_wayland`
- The *Mir* implementation of the wayland protocol.
- The `WaylandConnector` class is the glue between the compositor and the specific details of the wayland protocol.

## `/src/server/frontent_xwayland`
- The *Mir* implementation of the xwayland protocol.
- The `XWaylandConnector` class is the glue between the compositor and the specific details of the xwayland protocol.
- This protocol talks to the `xwayland` server, which is spawned as a subprocess.

## `/src/server/shell`
- The `Shell` is responsible for managing the surfaces that the compositor wants to show.
- It provides an interface to create, update, and remove these surfaces from the display.
- As an example, the `WaylandConnector` might as the `Shell` to create a new surface for it.

## `/src/server/scene`
- The `Scene` provides an interface for the `Compositor` to access the list of renderable items that are derived from the surfaces added to the `Shell`.
- Unliked the `Shell`, the `Scene` knows a lot about the Z-order of the surfaces. For this reason, it is also responsible for things like locking the display or sending surfaces to the back of the Z-order.

## `/src/server/graphics`
- An abstraction layer which connects the compositor to the specific graphics platform that its rendering on.

## `/src/server/console`
- Handles `logind` and virtual-terminal related tasks for the compositor

## `/src/server/report`
- Logging and other facilities to collect data about the compositor

## `/src/platforms` (with an "s"!)
- Provides both input and rendering platforms that the compositor can use depending on the environment.
- For the rendering platforms, we have:
  - *GBM/KMS*: the platform that you would expect if you were running a *Mir*-based compositor as your daily-driver
  - *X11*: a useful platform for testing locally
  - *Wayland*: another useful platform for testing locally
  - *EGLStreams/KMS*: like the *GBM/KMS* platform, but for Nvidia 
- For the input platforms, we have:
  - evdev
  - X11
  - Wayland

## `/src/platform` (no "s")
- Graphics and input code that is shared between platforms

## `/src/renderers` (with an "s"!)
- The supported `Renderer` types. The renderer is used by the Compositor composite the final image.
- Only GL is supported at the moment

## `/src/renderer` (no "s")
- Renderer code that is shared between renderers

## `/src/wayland`
- A subproject used to generate C++ classes from wayland protocol XML files.

## `/src/gl`
- A short list of helpers for GL work

## `/src/cookie`
- Provides event timestamps for inputs events that are difficult to spoof

## `/src/common`
- A library of common functionality that is used throughout the project, including things like loggers, clocks, executors, etc.

## `/examples`
- A collection of demo *Mir* compositors that are used as examples of how one might build a true compositor. Examples include:
  - `miral-app`
  - `miral-shel`
  - `mir_demo_server`

## `/tests`
- Unit tests for the entire project.