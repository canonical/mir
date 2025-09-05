# Architecture from the outside
This document introduces the architecture of *Mir* at a high-level.

## Audience
This document is intended to provide consumers of *Mir* an overview of *Mir*'s systems.

## APIs for compositor authors
```{mermaid} architecture-consumer-view.mmd
```

*Mir* provides compositor authors with a set of libraries that they can use to build Wayland based shells. These libraries are:

Library | Description
--      | --
*miral*|The most commonly used library is **miral** (the "*Mir* Abstraction Layer"). This provides an API that makes *Mir* easy for compositor authors to work with. It provides core window management concepts and an interface that is more ABI stable than Mir's internal APIs.
*mirwayland*|Compositor authors may want to define their own wayland protocol extensions in addition to the ones that the core *Mir* implementation defines. The **mirwayland** library satisfies this requirement. This library may be used in conjunction with either `miral` or `miroil`.
*miroil*|a custom library for the [Lomiri](https://lomiri.com/) folks. It is like **miral** in that it provides an abstraction over Mir's internal APIs. Most compositor authors will not interact with **miroil**.

These are supported by some low-level libraries (**mircore** and **mircommon**) that are also used by Mir's internal libraries. 

Library | Description
--      | --
*mircore*|Fundamental data concepts, like file descriptors and rectangles. These data structures tend not to be *Mir*-specific.
*mircommon*|*Mir*-specific data concepts like *Mir* event building, logging, and timing utilities.
