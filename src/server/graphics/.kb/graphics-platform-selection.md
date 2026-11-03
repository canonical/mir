# Preface

This file describes how Mir probes and selects graphics platform DSOs (display, rendering, and the buffer-allocating renderer) at startup, and the invariants that make selection deterministic.

Read the top-level `.kb/agents.md` file before continuing below.


# Laws

## §1 - Deterministic ordering

- [`select_display_modules()`](../platform_probe.cpp) sorts graphics DSOs by `dladdr` pathname (`dli_fname`) in reverse alphabetical order before probing, so `x11` precedes `wayland`. Equal support levels go to the first platform seen.
- Rendering selection probes [`libraries_for_path()`](../../../common/sharedlibrary/shared_library_prober.cpp) without pathname sorting; `select_libraries_for_path()` orders only by descending trailing `.so.<N>` version integer, leaving equal-versioned or unversioned DSOs in `directory_iterator` order.
- [`select_platforms_from_list()`](../platform_probe.cpp) iterates the DSO list rather than the user's comma-separated list, so manual probe order follows DSO scan order and throws `std::runtime_error` if any requested module name is missing.

## §2 - At most one nested platform

- Under automatic selection, platforms with no associated `udev::Device` are nested and only the single highest-rated one (`> probe::unsupported`) is retained. A manual platform list bypasses [`modules_for_device()`](../platform_probe.cpp) and can retain multiple deviceless platforms.

## §3 - Virtual is opt-in

- `mir:virtual` is excluded from normal display selection and appended only if a final reprobe reports a front device with `>= probe::supported`. A manually listed `mir:virtual` still runs in the manual loop for diagnostics: with `--virtual-output` its probe reports `supported` (suppressing the warning), the loop skips adding it (`platform != virtual_platform`), and the final reprobe appends it once; without `--virtual-output` its probe returns no devices, logging the try-anyway warning and adding nothing in either step.


# Concepts

## Selection pipeline

[`select_display_modules()`](../platform_probe.cpp) enumerates DSOs from `platform_path` (throwing `std::runtime_error` if empty), erases non-graphics modules lacking `describe_graphics_module`, sorts them (§1), then either probes a manual list or calls [`display_modules_for_device()`](../platform_probe.cpp).

[`modules_for_device()`](../platform_probe.cpp) swallows `std::runtime_error` from individual module probes, keeps the first platform claiming each hardware device (`> probe::unsupported`), and replaces it only on a strictly higher `support_level`. Device deduplication uses [`is_same_device()`](../platform_probe.cpp), which requires exact `udev::Device` equality (`devpath`) and treats a null device as equal to nothing. Throws `std::runtime_error` if neither hardware nor nested candidates survive.

## Support levels and type preference

[`probe::Result`](../../../../include/platform/mir/graphics/platform.h) is a `uint32_t`. [`modules_for_device()`](../platform_probe.cpp) ignores `unsupported` (0) claims and prunes hardware `<= probe::dummy` (1) claims only when at least one hardware claim exceeds `dummy` (if all hardware claims are `dummy`, all are kept).

[`display_modules_for_device()`](../platform_probe.cpp) passes `TypePreference::prefer_nested`: `best_nested` overrides non-empty hardware candidates only when `best_nested >= probe::supported` (128), though any `best_nested > unsupported` wins when hardware candidates are empty. [`rendering_modules_for_device()`](../platform_probe.cpp) checks `is_graphics_module` per DSO and passes `prefer_hardware`, where any non-empty hardware list (including `dummy`-only) wins over `best_nested`.

## Manual selection

Manual display selection ([`select_display_modules()`](../platform_probe.cpp)) appends devices with `>= probe::supported` (skipping `mir:virtual`, §3); when a listed platform reports none at `>= supported`, it logs a warning and appends all returned `SupportedDevice` entries anyway.

Manual rendering selection ([`the_rendering_platforms()`](../default_configuration.cpp)) appends every returned `SupportedDevice` unconditionally (including `< supported` and `unsupported`), using `>= probe::supported` only to suppress the warning.

## Platform instantiation and renderer selection

[`the_display_platforms()`](../default_configuration.cpp) and [`the_rendering_platforms()`](../default_configuration.cpp) instantiate selected platforms in order; if a constructor throws mid-loop, they format the exception into a local `std::stringstream` via `mir::report_exception()`, stop instantiating remaining entries, and succeed with the prefix already created (discarding the `error_report` unless none succeeded, in which case `std::runtime_error` is thrown).

`the_rendering_platforms()` groups created platforms by module name in a `std::multimap`, moving `mir:egl-generic` entries to the end. [`the_buffer_allocator()`](../default_configuration.cpp) narrows candidates to the first matching `MIR_PIN_COMPOSITING_TO` (matching `devnode` for hardware or module name for deviceless platforms, warning and falling back to all rendering platforms if unmatched), then calls [`select_buffer_allocating_renderer()`](../platform_probe.cpp). That helper acquires `GLRenderingProvider` per candidate, scores `suitability_for_display(sink)` across all `DisplaySink`s, filters to candidates minimizing `== probe::unsupported` sinks, and selects the first candidate maximizing `>= probe::best` sinks.

## Input stack coupling

Both [`the_display_platforms()`](../default_configuration.cpp) and [`the_rendering_platforms()`](../default_configuration.cpp) append loaded `SharedLibrary` handles to `platform_libraries`, which [`the_input_manager()`](../../input/default_configuration.cpp) later scans for input platforms (flagged as a TODO).
