pkg_check_modules(WLCS REQUIRED wlcs)
include_directories(${WLCS_INCLUDE_DIRS})

add_library(
  miral_wlcs_integration MODULE

  miral_integration.cpp
  test_wlcs_display_server.cpp test_wlcs_display_server.h
)

target_link_libraries(
  miral_wlcs_integration

  mir-test-assist
  ${WAYLAND_CLIENT_LDFLAGS} ${WAYLAND_CLIENT_LIBRARIES}
)

set(SYMBOL_FILE ${CMAKE_CURRENT_SOURCE_DIR}/symbol.map)

set_target_properties(
  miral_wlcs_integration PROPERTIES

  PREFIX ""
  LINK_FLAGS "-Wl,--version-script,${SYMBOL_FILE}"
  LINK_DEPENDS ${SYMBOL_FILE}
)

pkg_get_variable(WLCS_BINARY wlcs test_runner)

mir_discover_external_gtests(
  NAME wlcs
  COMMAND ${WLCS_BINARY}
  ARGS ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/miral_wlcs_integration.so
  EXPECTED_FAILURES
    ClientSurfaceEventsTest.frame_timestamp_increases
    ClientSurfaceEventsTest.surface_gets_enter_event
    ClientSurfaceEventsTest.surface_gets_leave_event
    SubsurfaceTest.place_above_simple
    SubsurfaceTest.place_below_simple
    TouchInputSubsurfaces/SubsurfaceMultilevelTest.subsurface_moves_after_both_sync_parent_and_grandparent_commit/0
    TouchInputSubsurfaces/SubsurfaceMultilevelTest.subsurface_with_desync_parent_moves_when_only_parent_committed/0
    TouchInputSubsurfaces/SubsurfaceTest.desync_subsurface_moves_when_only_parent_committed/0
    TouchInputSubsurfaces/SubsurfaceTest.one_subsurface_to_another_fallthrough/0
    TouchInputSubsurfaces/SubsurfaceTest.place_above_simple/0
    TouchInputSubsurfaces/SubsurfaceTest.place_below_simple/0
    TouchInputSubsurfaces/SubsurfaceTest.pointer_input_correctly_offset_for_subsurface/0
    TouchInputSubsurfaces/SubsurfaceTest.subsurface_does_not_move_when_parent_not_committed/0
    TouchInputSubsurfaces/SubsurfaceTest.subsurface_extends_parent_input_region/0
    TouchInputSubsurfaces/SubsurfaceTest.subsurface_moves_out_from_under_input_device/0
    TouchInputSubsurfaces/SubsurfaceTest.subsurface_moves_under_input_device_once/0
    TouchInputSubsurfaces/SubsurfaceTest.subsurface_moves_under_input_device_twice/0
    TouchInputSubsurfaces/SubsurfaceTest.sync_subsurface_moves_when_only_parent_committed/0
    WlShellSubsurfaces/SubsurfaceMultilevelTest.subsurface_moves_after_both_sync_parent_and_grandparent_commit/0
    WlShellSubsurfaces/SubsurfaceMultilevelTest.subsurface_with_desync_parent_moves_when_only_parent_committed/0
    WlShellSubsurfaces/SubsurfaceTest.desync_subsurface_moves_when_only_parent_committed/0
    WlShellSubsurfaces/SubsurfaceTest.one_subsurface_to_another_fallthrough/0
    WlShellSubsurfaces/SubsurfaceTest.place_above_simple/0
    WlShellSubsurfaces/SubsurfaceTest.place_below_simple/0
    WlShellSubsurfaces/SubsurfaceTest.pointer_input_correctly_offset_for_subsurface/0
    WlShellSubsurfaces/SubsurfaceTest.subsurface_does_not_move_when_parent_not_committed/0
    WlShellSubsurfaces/SubsurfaceTest.subsurface_extends_parent_input_region/0
    WlShellSubsurfaces/SubsurfaceTest.sync_subsurface_moves_when_only_parent_committed/0
    XdgShellStableSubsurfaces/SubsurfaceMultilevelTest.subsurface_moves_after_both_sync_parent_and_grandparent_commit/0
    XdgShellStableSubsurfaces/SubsurfaceMultilevelTest.subsurface_with_desync_parent_moves_when_only_parent_committed/0
    XdgShellStableSubsurfaces/SubsurfaceTest.desync_subsurface_moves_when_only_parent_committed/0
    XdgShellStableSubsurfaces/SubsurfaceTest.one_subsurface_to_another_fallthrough/0
    XdgShellStableSubsurfaces/SubsurfaceTest.place_above_simple/0
    XdgShellStableSubsurfaces/SubsurfaceTest.place_below_simple/0
    XdgShellStableSubsurfaces/SubsurfaceTest.pointer_input_correctly_offset_for_subsurface/0
    XdgShellStableSubsurfaces/SubsurfaceTest.subsurface_does_not_move_when_parent_not_committed/0
    XdgShellStableSubsurfaces/SubsurfaceTest.subsurface_extends_parent_input_region/0
    XdgShellStableSubsurfaces/SubsurfaceTest.sync_subsurface_moves_when_only_parent_committed/0
    XdgShellV6Subsurfaces/SubsurfaceMultilevelTest.subsurface_moves_after_both_sync_parent_and_grandparent_commit/0
    XdgShellV6Subsurfaces/SubsurfaceMultilevelTest.subsurface_with_desync_parent_moves_when_only_parent_committed/0
    XdgShellV6Subsurfaces/SubsurfaceTest.desync_subsurface_moves_when_only_parent_committed/0
    XdgShellV6Subsurfaces/SubsurfaceTest.one_subsurface_to_another_fallthrough/0
    XdgShellV6Subsurfaces/SubsurfaceTest.place_above_simple/0
    XdgShellV6Subsurfaces/SubsurfaceTest.place_below_simple/0
    XdgShellV6Subsurfaces/SubsurfaceTest.pointer_input_correctly_offset_for_subsurface/0
    XdgShellV6Subsurfaces/SubsurfaceTest.subsurface_does_not_move_when_parent_not_committed/0
    XdgShellV6Subsurfaces/SubsurfaceTest.subsurface_extends_parent_input_region/0
    XdgShellV6Subsurfaces/SubsurfaceTest.sync_subsurface_moves_when_only_parent_committed/0
    XdgSurfaceStableTest.attaching_buffer_to_unconfigured_xdg_surface_is_an_error
    XdgSurfaceStableTest.creating_xdg_surface_from_wl_surface_with_attached_buffer_is_an_error
    XdgSurfaceStableTest.creating_xdg_surface_from_wl_surface_with_committed_buffer_is_an_error
    XdgSurfaceStableTest.creating_xdg_surface_from_wl_surface_with_existing_role_is_an_error
    XdgToplevelStableTest.pointer_leaves_surface_during_interactive_move
    XdgToplevelStableTest.pointer_leaves_surface_during_interactive_resize
    XdgToplevelV6Test.pointer_leaves_surface_during_interactive_move
    XdgToplevelV6Test.pointer_leaves_surface_during_interactive_resize
)

install(TARGETS miral_wlcs_integration LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/mir)
