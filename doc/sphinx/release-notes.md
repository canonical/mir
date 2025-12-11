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
    - Implementation of an evdev platform in Rust (#4336, #4530)
    - Add support for ext_foreign_toplevel_list_v1 Wayland extension (#4343)
    - Implement wl_subsurface.place_above and place_below with parent z-ordering (#4483)
    - Add mg::Buffer::map_readable for CPU-readable buffer access (#4331)
    - cmake: extend/fix RustLibrary.cmake (#4528)
    - Bootstrap new release process and add Start Release workflow (#4519)
    - Modernise CMake dependencies and build configuration (#4500, #4474, #4475)
    - Use std::erase_if where appropriate for code modernization (#4510)
    - Add geometry::Value generic accessors for type-safe value conversion
    - Tidy up GL code and eliminate circular dependencies (#4475)

  - Bugs fixed:
    - Fix XWayland menu positioning by initializing spec from cached geometry (#4512)
    - Fix clip_area scaling with display scale factor (#4486)
    - Fix magnifier filter not consuming events at max/min magnifications (#4523)
    - Renderer: Ensure current EGL context when destroying GL resources (#4460)
    - Don't give up if specified cursor theme is not found (#4520)
    - Modify XCursorLoader::image to check the given name first (#4522)
    - Fix error message when clang import is not installed (#4497)
    - Replace usage of tmpnam() with mkdtemp() for safer temporary file handling (#4431, #4432)
    - Fix various TICS warnings in platforms and renderers
    - Remove `assert()` usage per coding guidelines (#4517)

  - Code cleanup and refactoring:
    - Remove dead code: scene::SurfaceEventSource and frontend::EventSink (#4397)
    - Tidy symbols map generation and check (#4502)
    - Drop workarounds for g++6 link failures (#4207)
    - Drop obsolete MIRSERVER_INCLUDE_DIRS and LinuxCrossCompile.cmake (#4473)
    - Move mir::report_exception() to mircore
    - Merge mir-renderer-gl-dev into mirplatform-dev

  - Testing improvements:
    - Add tests for SSD size constraints with different parameter combinations (#4468)
    - Add MIR_USING_WLCS_DEV option for WLCS development
    - XFAIL BadBufferTest.test_truncated_shm_file on wlcs <= 1.8.1

  - CI and tooling:
    - Add TICS debugging mode to collect debug info (#4477)
    - Fix TICS handling of cancelled runs (#4472)
    - Update Fedora 41 (EOL) references (#4491)

  - Documentation:
    - Add Release Notes skeleton and template
