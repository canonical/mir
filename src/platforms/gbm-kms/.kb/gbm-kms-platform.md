# Preface

GBM-KMS-specific behavior: the threaded page flipper, `GBMOutputSurface`, page-flip power
semantics, mode comparison, the surfaceless EGL context, and probe rules. Shared KMS invariants are in [`../../.kb/kms-display-invariants.md`](../../.kb/kms-display-invariants.md).

IMPORTANT: Read the `.kb/agents.md` file.


# Concepts

## Page flipper

- Exactly one thread becomes the worker that runs the `select` + `drmHandleEvent` loop. Other threads block
  on `pf_cv` until `worker_tid` clears or their CRTC completes ([`kms_page_flipper.cpp`](../server/kms/kms_page_flipper.cpp)).
- `clock_id` is `CLOCK_MONOTONIC` only when `DRM_CAP_TIMESTAMP_MONOTONIC` is set, otherwise
  `CLOCK_REALTIME`. Scheduling a flip for an already-pending CRTC throws `std::logic_error`.

## Page-flip power semantics

- `schedule_page_flip` returns `true` (success) when `power_mode != on`, so it reports a no-op as
  a successful flip. `wait_for_page_flip` returns early when powered off. Both functions acquire `power_mutex`,
  and `set_power_mode` holds it across `drmModeConnectorSetProperty` ([`real_kms_output.cpp`](../server/kms/real_kms_output.cpp)).
- `dpms_enum_id` is uninitialized unless a DPMS enum property is found during `reset()`. This creates
  a latent hazard ([`real_kms_output.h`](../server/kms/real_kms_output.h)).

## Output surface and formats

- `GBMOutputSurface` creates the EGL surface once at a fixed size and caches `surface_size`.
  `bind()` throws `std::logic_error` if the previous front buffer was not released. When no EGL
  configuration matches, the format falls back to its alpha or opaque equivalent DRM format
  ([`buffer_allocator.cpp`](../server/buffer_allocator.cpp)).
- `supported_pixel_formats()` returns only `argb`/`xrgb`, the conservative intersection of
  `ShmBuffer` and GBM. The code avoids `gbm_device_is_format_supported` because it has bugs ([LP: #1473901](https://bugs.launchpad.net/bugs/1473901)).
- `GBMDisplayAllocator` caches the DRM FB id in `gbm_bo` user data with a `drmModeRmFB` deleter
  ([`gbm_display_allocator.cpp`](../server/gbm_display_allocator.cpp)).

## Modes and configuration

- `kms_modes_are_equal` compares a subset of fields (clock, h/v display/start/end/total, hskew),
  and ignores type, name, vrefresh, and flags. `calculate_vrefresh_hz` derives Hz from
  clock/htotal/vtotal rather than the `vrefresh` field. `current_format` is hardcoded to `xrgb_8888`.
  Gamma is empty unless a current CRTC has `gamma_size > 0`, in which case it uses `LinearGammaLUTs` ([`real_kms_output.cpp`](../server/kms/real_kms_output.cpp)).
- `Display::configure_locked` waits for all pending page flips before rebuilding sinks.
  `RealKMSOutputContainer::update_from_hardware_state` reuses outputs, preserving
  `saved_crtc`, cursor, and page-flip state ([`display.cpp`](../server/kms/display.cpp), [`real_kms_output_container.cpp`](../server/kms/real_kms_output_container.cpp)). The O(n²) runtime is accepted (see matching rule in
  [`../../.kb/kms-display-invariants.md`](../../.kb/kms-display-invariants.md)).

## Surfaceless EGL context

- `SurfacelessEGLContext` is a surfaceless ES2 context used for sharing and allocator/EGL operations
  (display bind/unbind, resource import). `eglChooseConfig` requests one configuration and fails if none is returned ([`surfaceless_egl_context.cpp`](../server/surfaceless_egl_context.cpp), [`buffer_allocator.cpp`](../server/buffer_allocator.cpp)).
- `make_current` and `release_current` are `const` but mutate EGL global state. The destructor calls
  `release_current()` before `eglDestroyContext`. `release_current()` throws `egl_error` when
  `eglMakeCurrent` fails, and because it runs in the destructor an exception terminates the process;
  `eglDestroyContext` only returns a status.

## Probe rules

- The platform requires the EGL GBM platform. `llvmpipe` is unsupported, and the system skips the
  `nvidia` driver for display ([`platform_symbols.cpp`](../server/kms/platform_symbols.cpp)).
- The rendering probe prefers render nodes and skips a card node whose render node differs.
  `GBMDisplayProvider::is_same_device` uses `drmGetDeviceFromDevId` when compiled in, or node names otherwise ([`display.cpp`](../server/kms/display.cpp)).
