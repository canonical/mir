# Mir documentation

Mir is a compositor library designed to operate across a variety of Linux-based
devices, including traditional desktops, IoT and embedded systems.

Mir is a modern way to build Wayland compositors, providing
a well-defined, efficient, flexible and secure platform for graphical environments
that makes it an ideal choice for both device manufacturers and desktop users.

## In this documentation

These pages cover the key aspects of developing a compositor using Mir

````{grid} 1 1 2 2
```{grid-item-card} [Tutorials](tutorial-index)
**Start here** - hands-on introductions to Mir for new developers
- [Write Your First Wayland Compositor](/tutorial/write-your-first-wayland-compositor.md)
```

```{grid-item-card} [How-to guides](how-to-index)
**How-to guides** - step-by-step guides covering key operations and common tasks, including
[handling keyboard input](/how-to/how-to-handle-keyboard-input.md) and [integrating custom
wayland protocols](/how-to/how-to-integrate-a-custom-wayland-protocol.md)
```
````

````{grid} 1 1 2 2
---
reverse:
---
```{grid-item-card} [Reference](/reference/index.md)
**Technical information** - specifications, APIs, architecture:
[kernel requirements](/reference/kernel_requirements.md),
[introduction to the MirAL API](/reference/introducing_the_miral_api.md) and more
```

```{grid-item-card} [Explanation](/explanation/index.md)
**Discussion and clarification** of key topics, such as
[accessibility methods](/explanation/accessibility-methods.md),
[security](explanation/security.md) and more
```
````

## Project and community

Mir is a member of the Ubuntu family. It’s an open source project that warmly welcomes community projects, contributions, suggestions, fixes and constructive feedback.

- [Release Notes](https://github.com/canonical/mir/releases)
- [Code of conduct](https://ubuntu.com/community/docs/ethos/code-of-conduct)
- [Get support](https://discourse.ubuntu.com/c/project/mir/15)
- [Join our online chat](https://matrix.to/#/#mir-server:matrix.org)
- {ref}`Contribute <howto-contribute>`

Thinking about using Mir for your next project? [Get in touch](https://canonical.com/mir)!

```{toctree}
---
hidden:
---
tutorial/index
how-to/index
explanation/index
reference/index
configuring/index
contributing/index
```
