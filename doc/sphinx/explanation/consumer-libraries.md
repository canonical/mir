# User libraries

These are the principle libraries *Mir* provides compositor authors.

These libraries will not break ABI or API between minor versions of Mir.
We do not provide the same guarantees for other, internally used, libraries.

## APIs for compositor authors

```{mermaid} consumer-libraries.mmd
```

*Mir* provides compositor authors with a set of libraries that they can use to build Wayland based shells. These libraries are:

| Library      | Description                                                                                                                                                                                                                                                                     |
| ------------ | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| *miral*      | The most commonly used library is **miral** (the "*Mir* Abstraction Layer"). This provides an API that makes *Mir* easy for compositor authors to work with. It provides core window management concepts and an interface that is more ABI stable than Mir's internal APIs.     |
| *mirwayland* | Compositor authors may want to define their own wayland protocol extensions in addition to the ones that the core *Mir* implementation defines. The **mirwayland** library satisfies this requirement. This library may be used in conjunction with either `miral` or `miroil`. |
| *mircore*    | Fundamental data concepts, like file descriptors and rectangles. These data structures tend not to be *Mir*-specific.                                                                                                                                                           |
