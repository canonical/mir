# Preface

Shared KMS display invariants for the atomic-kms and gbm-kms graphics platforms: framebuffer
lifetimes, cursor handling, configuration compatibility, driver quirks, and CRTC save/restore.
Platform-specific behavior lives in `atomic-kms/.kb/atomic-kms-platform.md` and
`gbm-kms/.kb/gbm-kms-platform.md`.

Read the top-level `.kb/agents.md` file before continuing below.


# Laws

## §1 - Framebuffer lifetime

KMS takes no reference to a submitted framebuffer. Keep the `next_swap`/`scheduled_fb`/`visible_fb`
`shared_ptr` chain alive across `post()`. Destroying an in-use FB makes KMS treat it as null and
blank the display ([`atomic`](../atomic-kms/server/kms/display_sink.h), [`gbm`](../gbm-kms/server/kms/display_sink.h)).

## §2 - CRTC restore

The pre-existing CRTC is restored on destruction only while `using_saved_crtc == false`. `set_crtc`
clears the flag on both backends. Only atomic page-flip submission also clears it, while GBM
`schedule_page_flip` does not. `clear_crtc` never touches it ([`atomic`](../atomic-kms/server/kms/atomic_kms_output.cpp), [`gbm`](../gbm-kms/server/kms/real_kms_output.cpp)).


# Concepts

## Cursor

- The system leaves the `drmModeSetCursor2` hotspot unused because it fails on radeon and intel drivers, following weston precedent.
  The hotspot is folded into the position passed to `move_cursor` ([`atomic`](../atomic-kms/server/kms/cursor.cpp), [`gbm`](../gbm-kms/server/kms/cursor.cpp)).
- Cursor BOs are cached per `(output id, drm_fd)` and sized from `DRM_CAP_CURSOR_{WIDTH,HEIGHT}`.
  Software handles rotation and padding before upload.

## Configuration compatibility

- Two configurations are compatible only if the card and output count match, and each output's
  `power_mode` matches. The outputs must also match after ignoring orientation, scale, form factor,
  subpixel arrangement, and custom logical size. A power-state change forces buffer recreation
  ([`atomic`](../atomic-kms/server/kms/real_kms_display_configuration.cpp), [`gbm`](../gbm-kms/server/kms/real_kms_display_configuration.cpp)).
- Output IDs are positional (`index + 1`) and unstable across hotplug. Output containers match
  hardware by `(connector_id, drm_fd)` ([`atomic`](../atomic-kms/server/kms/real_kms_output_container.cpp), [`gbm`](../gbm-kms/server/kms/real_kms_output_container.cpp)).

## Driver quirks

- The default block list is `{ast, simple-framebuffer}`. The default `disable-kms-probe` list is
  `{virtio_gpu, vc4-drm, v3d}` ([`atomic`](../atomic-kms/server/kms/quirks.cpp), [`gbm`](../gbm-kms/server/kms/quirks.cpp)).
- `MIR_MESA_KMS_DISABLE_MODESET_PROBE` and `MIR_GBM_KMS_DISABLE_MODESET_PROBE` both force
  `require_modesetting_support()` to false.
- `gbm-surface-has-free-buffers:...:skip` (forced true) is available on both platforms. The
  `egl-destroy-surface:...:leak` quirk exists only in gbm-kms.

## Hardware quirks

- VMWare fallback: if no current mode matches, the output selects a mode named `preferred`
  (bug 1661295) ([`atomic`](../atomic-kms/server/kms/atomic_kms_output.cpp), [`gbm`](../gbm-kms/server/kms/real_kms_output.cpp)).
- EDID is read only for connected outputs. An EDID property value (blob ID) of `0` means no EDID,
  which is logged at debug level.
- `ensure_crtc()` re-fetches the connector before `find_crtc` (issue #2661).
- `clear_crtc()` treats `EACCES`/`EPERM` (logind revoked on VT switch) as non-fatal.
