---
myst:
  html_meta:
    description: Planned improvements and longer-term work for Mir.
---

(roadmap)=

# Roadmap

This page summarizes planned improvements for Mir for the given Ubuntu release cycles.
The roadmap is exploratory and may change as priorities, designs, and implementation details develop.

(cycle-26-10)=

## 26.10

### Use a hardware plane for video playback

Use display planes for eligible video subsurfaces when the hardware supports them.
This can reduce GPU load by only updating the video plane, and with no format conversions.
Client subsurfaces, colour formats, refresh rates, and hardware capabilities will help identify candidates.

### Support building a Mir-based compositor in Rust

Expose an idiomatic, MirAL-like Rust API for building a Mir-based compositor without C++.
The API will cover applicable MirAL functionality, include a documented example compositor, and be built and tested in CI.
This should also make it practical to write a small tiling window manager in Rust.

### Review and stabilise the mir-shell Wayland extension

Define and document mir-shell protocol extension v2, based on meaningful feedback from a client such as Flutter.
The v2 protocol should cover the Flutter client's window-management needs without workarounds.

### Place foreign window contents from shell components

Provide a privileged protocol allowing shell components such as app switchers and expose effects to embed other clients'
toplevel surfaces in their own surface trees, with simple transformations such as scaling and translation.
The design should align with `wp_viewporter` and subsurface protocols.

### Design a long-term Mir rendering pipeline

Agree on a long-term rendering pipeline specification that balances compositor flexibility with Mir's control of rendering.
It will cover animations, window effects, window management effects, input routing,
and the relationship between the renderer implementation (GL, Vulkan, SoC-optimised, or software) and those features.

### Build an `OutputConfigurationPolicy` mechanism

Add a public MirAL customization point, analogous to `BasicWindowManager` and `WindowManagementPolicy`, for output configuration.
Third-party developers should be able to implement policy logic using MirAL abstractions only, and have it invoked throughout the output configuration lifecycle.

### Expose interfaces for custom rendering requirements

Expose stable interfaces that allow a custom renderer module to be loaded at runtime and supplied by a custom GPU support snap.

### Shell security best practices

Provide shell authors with documentation and tools for building baseline-secure shells.
Guidance will cover granting specific protocols to specific clients, the security impact of privileged protocols, protocols
that should not be exposed to sandboxed applications, client identification, and possible use of the Wayland security-context protocol.

### Mir shell systemd integration

Provide documentation, example systemd units, and code where needed to help shell authors set up fully functional systemd user sessions.
The compositor should signal readiness before graphical session components start and support correct shutdown behaviour.
