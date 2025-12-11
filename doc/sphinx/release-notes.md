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
    - Implement StandardApplicationSwitcher for ease of use (#4382)
    - cmake: extend/fix RustLibrary.cmake (#4528)
    - Bootstrap new release process and add Start Release workflow (#4519)
    - Modernise CMake dependencies and build configuration (#4500, #4474, #4475, #4442, #4441, #4459, #4462, #4465)
    - Use std::erase_if and std::ranges::reverse_view for code modernization (#4510, #4458)
    - Add geometry::Value generic accessors for type-safe value conversion
    - Tidy up GL code and eliminate circular dependencies (#4475)
    - Improve cursor scale animation (#4434)
    - Check DRM_CAP_SYNCOBJ_TIMELINE before providing DRMRenderingProvider (#4437)
    - Add experimental platform priority for input platforms (#4455)

  - Bugs fixed:
    - Fix XWayland menu positioning by initializing spec from cached geometry (#4512)
    - Fix clip_area scaling with display scale factor (#4486)
    - Fix magnifier filter not consuming events at max/min magnifications (#4523)
    - Fix cursor size getting truncated after scaling (#4433)
    - Fix LocatePointer being invoked with extra unintended keys pressed (#4380)
    - Fix discarded-qualifiers error in xcursor.c on Fedora Rawhide (#4467)
    - Fix mold undefined reference issues with GL libraries (#4409)
    - Fix high-impact compiler warnings from TICS analysis (#4443)
    - WaylandExecutor::spawn() - do not try to lock the queue unless the execution state is Running (#4469)
    - Ensure ApplicationSwitcher always has CopyAssign symbols defined (#4436)
    - Unload the input platform after all other destruction has happened (#4429)
    - Renderer: Ensure current EGL context when destroying GL resources (#4460)
    - Don't give up if specified cursor theme is not found (#4520)
    - Modify XCursorLoader::image to check the given name first (#4522)
    - Fix error message when clang import is not installed (#4497)
    - Replace usage of tmpnam() with mkdtemp() for safer temporary file handling (#4431, #4432)
    - Fix various TICS warnings in platforms and renderers
    - Remove `assert()` usage per coding guidelines (#4517)

  - Code cleanup and refactoring:
    - Remove dead code: scene::SurfaceEventSource and frontend::EventSink (#4397)
    - Remove dead code: DisplayChanger preview/confirm methods (#4447)
    - Remove frontend::Surface::wayland_surface() (#4371)
    - Remove unused directory and empty build rules (#4413)
    - Drop dead headers (#4454)
    - Tidy symbols map generation and check (#4502)
    - Drop workarounds for g++6 link failures (#4207)
    - Drop obsolete MIRSERVER_INCLUDE_DIRS and LinuxCrossCompile.cmake (#4473)
    - Cleanup include file handling in mirserver (#4500)
    - Fix include style (#4450)
    - Clean up some cruft (#4445)
    - Move mir::report_exception() to mircore
    - Merge mir-renderer-gl-dev into mirplatform-dev
    - Publish mirplatform headers used outside mirplatform (#4499)
    - Route security_log through logging infrastructure to respect test configuration (#4399)
    - Providing the user with useful errors in the symbols map generator (#4404)
    - Migrate all MOCK_METHOD[0-9] to MOCK_METHOD syntax and add override keyword (#4418)
    - Const& a bunch of shared_ptr parameters (#4416)
    - Spelling fixes (#4412)
    - create_data_source: Capture state instead of accessing it via this (#4407)

  - Testing improvements:
    - Add tests for SSD size constraints with different parameter combinations (#4468)
    - Add MIR_USING_WLCS_DEV option for WLCS development
    - XFAIL BadBufferTest.test_truncated_shm_file on wlcs <= 1.8.1
    - Refresh WLCS expected failures (#4405)

  - CI and tooling:
    - Add TICS debugging mode to collect debug info (#4477)
    - Fix TICS handling of cancelled runs (#4472)
    - Provide clang deps for TICS (#4408)
    - Modernize coverage setup (#4444)
    - Update Fedora 41 (EOL) references (#4491)
    - Spread: Fedora 43 is out (#4414)
    - Update actions/upload-artifact action to v5 (#4381)
    - Tools: prune (#4400)

  - Documentation:
    - Add Release Notes skeleton and template
    - Add .github/copilot-instructions.md for AI coding agent guidance (#4422)
    - Update the input platform explanation to include information about threading (#4438)
    - Fix link to Mir website (#4402)
    - Fix link (#4435)
