# Architecture
This document introduces the architecture of *Mir* at a high-level.

## Audience
This document is intended to provide contributors to *Mir* an overview of *Mir*'s systems. It is *not* intended to guide compositor authors.

Click [here](./architecture_diagram.rst) to see a diagram of *Mir*'s systems.

## Index
- [APIs for compositor authors](#apis-for-compositor-authors)
- [The Mir Engine](#the-mir-engine)
- [Platforms](#platforms)
- [Supporting Libraries](#supporting-libraries)

## APIs for compositor authors
*Mir* provides compositor authors with a set of libraries that they can use to build Wayland based shells. These libraries are:
- *miral*
- *miroil*
- *mirwayland*

### Miral
The most commonly used library is `miral`. `miral` (the "*Mir* Abstraction Layer") is an API that makes *Mir* easy for compositor authors to work with. It provides core window management concepts and an interface that is more ABI stable than Mir's internal API. `miral` is built on the [*Mir* engine](#the-mir-engine), however compositor authors are encouraged to only interact with the high-level `miral` library.

### Miroil
`miroil` is a custom library for the [Lomiri](https://lomiri.com/) folks. Most compositor authors will not interact with `miroil`. [The *Mir* engine](#the-mir-engine) also serves as the basis for `miroil` as it does for `miral`.

### Mirwayland
Compositor authors may want to define their own wayland protocol extensions in addition to the ones that the core *Mir* implementation defines. The `mirwayland` library satisfies this requirement. This library may be used in conjunction with either `miral` or `miroil`.

## The Mir engine
The `mirserver` library is the core implementation of *Mir*. It serves as the engine for both `miral` and `miroil`. This library does the heavy-lifting for the compositor and, as a result, it is the largest piece of *Mir*. This section will explain the primary concepts that drive the engine.

### Core Concepts
At the heart of `mirserver` are two interfaces: the **Shell** and the **Scene**. The `Shell` is responsible for fielding requests from the rest system. The `Shell` then modifies the state of the `Scene` to reflect the requested changes. For example, the `Frontend` would ask the `Shell` to initiate dragging a window. The `Shell` would then decide how to move that window to and update the state of the `Scene` to reflect that change.

### From Scene to Screen
Knowing that the `Scene` holds the state of what is to be displayed, we can talk about the **Compositor**. The `Compositor` gets the collection of items to render from the `Scene`,
renders them, and sends them off to the [platform](#platforms) to be displayed.

### From Interaction to Shell
As stated previously, the `Shell` handles requests from the system and updates the state of the `Scene` accordingly. These requests come from a variety of sources, which we will investigate now.

**Frontend Wayland**: Responsible for connecting the Wayland protocol with the rest of *Mir*. The `WaylandConnector` class connects requests made via the Wayland protocol to the core state of the compositor.

**Frontend XWayland**: Responsible for connecting requests send by an `XWayland` server to the rest of *Mir*. The `XWaylandConnector` establishes this connection just like the `WaylandConnector`. This frontend actually spawns the `XWayland` server as a subprocess.

**Input**: Handles everything having to do with input, including mouse, keyboard, touch, and more. This piece of *Mir* interacts with the specific input [platform](#platforms) and bubbles up events through a list of `InputDispatcher`s, which enables different pieces of the software to react to the events. For example, a compositor's window manager may respond to a key press event by opening up a new terminal via a request to the `Shell`.

## Platforms
We briefly hinted at the existence of so-called "platforms" previously, but they are deserving of a dedicated section. A **Platform** is an adapter that allows the system to work across different graphics, input, and rendering stacks. They come in three flavors:
- **Display Platform**: Determines what the compositor is rendering to. This may be to a physical monitor via GBM/KMS (or EGLStreams for Nvidia), to an X11 or Wayland window, or to a completely virtual buffer.
- **Input Platform**: Determines where the compositor is getting input from. This could be native event via `libinput`, X input events, or Wayland input events.
- **Rendering Platform**: Determines how the compositor renders the final image. For now, only a GL backend is supported here.

The GBM/KMS platform is most typically what will be used, as it offers a native platform. The X11 platform is useful for development. The Wayland platform is specifically useful for Ubuntu Touch, where they are hosting *Mir* in another Wayland compositor.

## Supporting Libraries
*Mir* leans on a few core libraries to support the entire system. These libraries contain data structures and utilities that are shared throughout the project, including `miral` and `miroil`.

- **Core**: Fundamental data concepts, like file descriptors and rectangles. These data structures tend not to be *Mir*-specific.
- **Common**: *Mir*-specific data concepts like *Mir* event building, logging, and timing utilities.