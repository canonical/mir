# Preface

Atomic-KMS-specific behavior: the `AtomicUpdate` request builder, atomic probe/init requirements,
GBM surface and framebuffer handling, and output reset/refresh. Shared KMS invariants (framebuffer
lifetimes, cursor, configuration compatibility, quirks) are in
[`../../.kb/kms-display-invariants.md`](../../.kb/kms-display-invariants.md).

IMPORTANT: Read the `.kb/agents.md` file.


# Laws

## §1 - Atomic property errors are fatal

`AtomicUpdate::add_property` aborts through `MIR_FATAL_ERROR` with no recovery. A missing DRM property
is a programmer error. A `drmModeAtomicAddProperty` failure is a programmer error or memory
exhaustion ([`atomic_update.cpp`](../server/kms/atomic_update.cpp)).


# Concepts

## AtomicUpdate

- `commit()` returns `0` on success or a negative errno on failure ([`atomic_update.h`](../server/kms/atomic_update.h)).
- `set_crtc`, `clear_crtc`, `set_power_mode`, and `set_gamma` commit with
  `DRM_MODE_ATOMIC_ALLOW_MODESET`. `page_flip` commits with flags `0` ([`atomic_kms_output.cpp`](../server/kms/atomic_kms_output.cpp)).
- If the submitted FB size differs from the CRTC size, `page_flip` rejects the request. The caller falls
  back to `set_crtc` because a modeset cannot be a pageflip.

## Probe and init

- The platform requires the `EGL_KHR_platform_gbm` or `EGL_MESA_platform_gbm` client extension, and
  `DRM_CLIENT_CAP_ATOMIC`. It sets `DRM_CLIENT_CAP_UNIVERSAL_PLANES` on the master fd
  ([`platform_symbols.cpp`](../server/kms/platform_symbols.cpp), [`platform.cpp`](../server/kms/platform.cpp)).
- An `llvmpipe` renderer is unsupported. `probe::best` requires connectors, CRTCs, and encoders all
  `> 0`. `drmGetBusid()` returns null unless `drmSetInterfaceVersion(1.4)` ran first.

## GBM buffers

- GBM surface creation retries without flags when the flagged creation fails (NVIDIA before 575)
  ([`gbm_display_allocator.cpp`](../server/gbm_display_allocator.cpp)).
- `GBMBoFramebuffer` caches the DRM FB id in `gbm_bo` user data. `DmaBufDisplayAllocator` caps at
  4 planes, and captures the `gbm_bo` and buffer to keep the FB alive ([`display_buffer.cpp`](../server/kms/display_buffer.cpp)).
- `formats_for_output` deduplicates consecutive formats from the `IN_FORMATS` blob (modifiers are grouped
  per format). It returns `ARGB8888`/`XRGB8888` only when the primary plane has no `IN_FORMATS` property;
  if the property exists but the iterator yields no formats, the result is empty
  ([`atomic_kms_output.cpp`](../server/kms/atomic_kms_output.cpp)).

## Output lifecycle

- A `set_crtc` failure is non-fatal (VirtualBox resize) ([`display_buffer.cpp`](../server/kms/display_buffer.cpp)).
- `reset()` releases the CRTC and plane for disconnected outputs. `refresh_hardware_state()`
  reprobes the connector and re-derives the CRTC from the encoder ([`atomic_kms_output.cpp`](../server/kms/atomic_kms_output.cpp)).
- `Display::configure_locked` does not wait for page flips (atomic has no page flipper)
  ([`display.cpp`](../server/kms/display.cpp)).
