# Release Notes

<!--

## Mir X.YY.Z

- New upstream release X.YY.Z

  - ABI summary:

    - miral ABI unchanged at <version>
    - mircommon ABI unchanged at <version>
    - mircore ABI bumped to <version>
    - miroil ABI unchanged at <version>
    - mirplatform ABI bumped to <version>
    - mirserver ABI unchanged at <version>
    - mirwayland ABI bumped to <version>
    - mirplatformgraphics ABI unchanged at <version>
    - mirinputplatform ABI bumped to <version>

  - Enhancements:

    - Pull request title (#<PR number>)

  - Bugs fixed:

    - Issue title (#<Issue number>)

  - Documentation:

    - Pull request title (#<PR number>)

-->

## Mir 2.27.0~dev

- New upstream release 2.27.0

  - ABI summary:

    - miral ABI unchanged at 7
    - mircommon ABI unchanged at 12
    - mircore ABI bumped to 3
    - miroil ABI unchanged at 8
    - mirplatform ABI unchanged at 34
    - mirserver ABI unchanged at 67
    - mirwayland ABI bumped to 6
    - mirplatformgraphics ABI unchanged at 23
    - mirinputplatform ABI unchanged at 10

## Mir 2.26.0

- New upstream release 2.26.0

  - ABI summary:

    - miral ABI unchanged at 7
    - mircommon ABI bumped to 12
    - mircore ABI unchanged at 2
    - miroil ABI unchanged at 8
    - mirplatform ABI unchanged at 34
    - mirserver ABI bumped to 67
    - mirwayland ABI unchanged at 5
    - mirplatformgraphics ABI unchanged at 23
    - mirinputplatform ABI unchanged at 10

  - Enhancements:

    - [evdev-rs] Implementing touch events in the evdev-rs platform [#4671](https://github.com/canonical/mir/pull/4671)
    - [evdev-rs] Fix missing initial events in evdev-rs [#4780](https://github.com/canonical/mir/pull/4780)
    - [evdev-rs] Add input event reporting to the evdev-rs input platform [#4769](https://github.com/canonical/mir/pull/4769)
    - [evdev-rs] Cleanup of the device module in evdev-rs [#4722](https://github.com/canonical/mir/pull/4722)
    - [evdev-rs] Fix evdev-rs pointer events by moving button state to per-device storage [#4680](https://github.com/canonical/mir/pull/4680)
    - [evdev-rs] Refactor the event processing logic in evdev-rs to increase clarity [#4740](https://github.com/canonical/mir/pull/4740)
    - [wayland-rs] Started implementing Wayland frontend in Rust
    - Allow CursorObservers to monitor when the cursor image changes [#4579](https://github.com/canonical/mir/pull/4579)
    - Add requires clauses to libmiral templates [#4764](https://github.com/canonical/mir/pull/4764)
    - [Wayland] ext-input-triggers - MVP (V1) [#4328](https://github.com/canonical/mir/pull/4328)
    - [Wayland] Implement ext-input-triggers [#4410](https://github.com/canonical/mir/pull/4410)
    - [Wayland] Update `ext-input-trigger-action-v1.xml` to add more details about token validity and availability [#4770](https://github.com/canonical/mir/pull/4770)
    - [Wayland] Partial implementation of ext_image_copy_capture_v1 cursor sessions [#4632](https://github.com/canonical/mir/pull/4632)
    - [Wayland] Send the cursor image via `ext_image_copy_capture_v1` [#4685](https://github.com/canonical/mir/pull/4685)
    - feature: replace `strerror()` with thread safe version(`strerror_r()`) [#4590](https://github.com/canonical/mir/pull/4590)
    - feature: Move logging APIs (and defaults) to mircore [#4644](https://github.com/canonical/mir/pull/4644)
    - CursorObserverMultiplexer sends the initial state to newly registered CursorObservers [#4662](https://github.com/canonical/mir/pull/4662)
    - Refactor `MinimalWindowManager` for better specialization [#4666](https://github.com/canonical/mir/pull/4666)
    - Add the ability to set the alpha of a Window and retrieve it via WindowInfo [#4704](https://github.com/canonical/mir/pull/4704)

  - Bugs fixed:

    - BasicXCBConnection::destroy_window calls xcb_map_window instead of xcb_destroy_window [#4560](https://github.com/canonical/mir/issues/4560)
    - XCB replies not always freed [#4559](https://github.com/canonical/mir/issues/4559)
    - Making videos fullscreen in Google Chrome fails if the window isn't already fullscreen [#4688](https://github.com/canonical/mir/issues/4688)
    - [X11] Set `_NET_CLIENT_LIST_STACKING` [#2023](https://github.com/canonical/mir/issues/2023)
    - Fatal signal handling is wildly unsafe [#2658](https://github.com/canonical/mir/issues/2658)
    - `DRM_CLIENT_CAP_ATOMIC` check is performed with wrong ioctl call [#4774](https://github.com/canonical/mir/issues/4774)
    - Clean up and improve `mgk::find_crtc_with_primary_plane` [#3972](https://github.com/canonical/mir/issues/3972)
    - If a surface has an opaque region BUT the alpha is not 1.0, then we shouldn't use it as an occlusion [#4696](https://github.com/canonical/mir/issues/4696)

  - New Contributors

    - @DiegoOst made their first contribution in https://github.com/canonical/mir/pull/4572
    - @magedss made their first contribution in https://github.com/canonical/mir/pull/4582
    - @aliihsancengiz made their first contribution in https://github.com/canonical/mir/pull/4591
    - @thomaskroi1996 made their first contribution in https://github.com/canonical/mir/pull/4576
    - @DigraJatin made their first contribution in https://github.com/canonical/mir/pull/4593
    - @pierre-l made their first contribution in https://github.com/canonical/mir/pull/4697
    - @yotam-medini made their first contribution in https://github.com/canonical/mir/pull/4750
    - @krzysztof-buczak-red made their first contribution in https://github.com/canonical/mir/pull/4776

## Mir 2.25.2

- Bugs fixed:
  - PkgConf isn't finding xkbcommon.h on openSUSE Tumbleweed [#4544](https://github.com/canonical/mir/pull/4544)
  - libmirplatform-dev does not upgrade if mir-renderer-gl-dev is installed [#4563](https://github.com/canonical/mir/pull/4563)

## Mir 2.25.1

- Bug fix for PkgConf files missing versions [#4544](https://github.com/canonical/mir/pull/4544)

## Mir 2.25.0

- New upstream release 2.25.0

  - ABI summary:

    - miral ABI unchanged at 7
    - mircommon ABI unchanged at 11
    - mircore ABI unchanged at 2
    - miroil ABI unchanged at 8
    - mirplatform ABI bumped to 34
    - mirserver ABI bumped to 66
    - mirwayland ABI unchanged at 5
    - mirplatformgraphics ABI unchanged at 23
    - mirinputplatform ABI unchanged at 10

  - Enhancements:

    - [Wayland] Add support for the ext_foreign_toplevel_list_v1 extension
    - [Wayland] Implement ext-data-control
    - [Wayland] Partial implementation of ext-image-capture-source and
      ext-image-copy-capture Wayland extensions
    - Rework wayland extension management so that connections originating from
      the server can always use the required extensions
    - Implementation of an evdev platform in rust
    - Added copilot instructions
    - Roll mir-renderer-gl (et alia) into mirplatform
    - Publish the mirplatform headers used outside mirplatform
    - Improve Cursor Scale animation
    - Implement LocatePointer
    - Implement ApplicationSwitcher
    - Implement the StandardApplicationSwitcher for ease of use
    - New FloatingWindowManager and deprecate MinimalWindowManager
    - Handle opaque regions of transparent surfaces
    - Log security events according to OWASP format
    - Add geometry::Value generic accessors for type-safe value conversion
    - Move mir::report_exception() to mircore

  - Bugs fixed:

    - Cursor icons don't change with XTerm (#4134)
    - Fix magnifier filter not consuming events when magnifications is
      greater/less than the max/min (#4386)
    - If no specified cursor theme loads, then internal theme (#439)
    - Renderer: Ensure current EGL context when destroying GL resources (#4460)
    - Fix XWayland menu positioning by initializing spec from cached geometry
      (#4512)
    - [Wayland] Implement wl_subsurface.place_above and place_below with parent
      z-ordering (#4332)
    - WindowInfo::clip_area is incorrect if the output is scaled (#4484)
    - Check DRM_CAP_SYNCOBJ_TIMELINE in platform layer before providing
      DRMRenderingProvider
    - Fix cursor size getting truncated after scaling leading to crash (#4377)
    - Intermittent crash (#4323)

  - Documentation:

    - Add Release Notes template
    - Update the input platform explanation to include information about threading
    - Refresh some links
