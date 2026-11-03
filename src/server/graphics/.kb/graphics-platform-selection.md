# Preface

This file describes how Mir probes and selects graphics platform DSOs (display and rendering) at startup, and the invariants that make that selection deterministic.

Read the top-level `.kb/agents.md` file before continuing below.


# Laws

## §1 - Deterministic ordering

Display selection sorts platform DSOs by filename in reverse alphabetical order before probing, so `x11` is preferred over `wayland`. Equal support levels go to the first platform seen. Rendering selection probes the unsorted `libraries_for_path()` list.

## §2 - At most one nested platform

Under automatic selection, platforms with no associated `udev` device are nested and only the single best-supported one is selected. A manual platform list bypasses this rule and can retain several.

## §3 - Virtual is opt-in

`mir:virtual` is excluded from normal selection and appended only if it is reprobed and its front device reports at least `probe::supported`. A manually specified virtual platform is skipped in the manual loop and goes through that same final probe: with `--virtual-output` the probe reports `supported` and the platform is added; without it, the probe returns no devices and nothing is added.


# Concepts

## Selection pipeline

[`select_display_modules()`](../platform_probe.cpp) enumerates DSOs from `platform_path`, drops non-graphics modules, sorts them (§1), then either probes a user-specified list or calls [`display_modules_for_device()`](../platform_probe.cpp).

[`modules_for_device()`](../platform_probe.cpp) is the shared core: it keeps the first platform claiming a device and replaces it only on a strictly higher `support_level`. Device deduplication uses [`is_same_device()`](../platform_probe.cpp), which requires exact `udev` path equality and treats a null device as equal to nothing.

## Support levels

[`probe::Result`](../../../../include/platform/mir/graphics/platform.h) is a `uint32_t`. Automatic selection never adds `unsupported` (0) devices and prunes `dummy` (1) platforms whenever any non-dummy platform exists. The `supported` (128) level gates the virtual platform.

## Manual selection

A manually specified platform list bypasses [`modules_for_device()`](../platform_probe.cpp). Display selection adds each listed platform's supported devices, or all probed devices through the try-anyway fallback when it claims none; the virtual platform is skipped in that loop and probed by the same final probe as automatic selection (§3). Rendering selection appends every probed device. Therefore, the system can retain several deviceless platforms and can add unsupported devices.

## Type preference

[`display_modules_for_device()`](../platform_probe.cpp) uses `TypePreference::prefer_nested`. [`rendering_modules_for_device()`](../platform_probe.cpp) uses `prefer_hardware` and requires an `is_graphics_module` (`describe_graphics_module`) symbol before probing.

## Rendering platform assembly

[`the_rendering_platforms()`](../default_configuration.cpp) groups created platforms by module name in a `std::multimap` and pushes `mir:egl-generic` last as a fallback. `MIR_PIN_COMPOSITING_TO` matches the devnode, or the module name for deviceless platforms.

## Input stack coupling

Both [`the_display_platforms()`](../default_configuration.cpp) and [`the_rendering_platforms()`](../default_configuration.cpp) append their modules to `platform_libraries`, which [`the_input_manager()`](../../input/default_configuration.cpp) later searches. The code flags this coupling as a TODO.
