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
