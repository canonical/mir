pkg_check_modules(WLCS REQUIRED IMPORTED_TARGET wlcs)

add_library(
  miral_wlcs_integration MODULE

  miral_integration.cpp
  test_wlcs_display_server.cpp test_wlcs_display_server.h
)

target_link_libraries(
  miral_wlcs_integration

  mir-test-assist
  PkgConfig::WAYLAND_CLIENT
)

target_include_directories(
  miral_wlcs_integration

  PRIVATE
    PkgConfig::WLCS
)

set(SYMBOL_FILE ${CMAKE_CURRENT_SOURCE_DIR}/symbol.map)

set_target_properties(
  miral_wlcs_integration PROPERTIES

  PREFIX ""
  LINK_FLAGS "-Wl,--version-script,${SYMBOL_FILE}"
  LINK_DEPENDS ${SYMBOL_FILE}
)

pkg_get_variable(WLCS_BINARY wlcs test_runner)

if(cmake_build_type_lower MATCHES "addresssanitizer")
  set(WLCS_BINARY ${WLCS_BINARY}.asan)
elseif(cmake_build_type_lower MATCHES "threadsanitizer")
  set(WLCS_BINARY ${WLCS_BINARY}.tsan)
elseif(cmake_build_type_lower MATCHES "ubsanitizer")
  set(WLCS_BINARY ${WLCS_BINARY}.ubsan)
endif()

set(EXPECTED_FAILURES

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

  AllSurfaceTypes/TouchTest.sends_touch_up_on_surface_destroy/4 # Fixed by https://github.com/MirServer/wlcs/pull/199
  AllSurfaceTypes/TouchTest.sends_touch_up_on_surface_destroy/5 # Fixed by https://github.com/MirServer/wlcs/pull/199

  ForeignToplevelHandleTest.can_maximize_foreign_while_fullscreen # https://github.com/MirServer/mir/issues/2164

  # See https://github.com/MirServer/mir/issues/2324
  TextInputV3WithInputMethodV2Test.text_input_enters_parent_surface_after_child_destroyed
  TextInputV3WithInputMethodV2Test.text_input_enters_grabbing_popup
  XdgPopupUnstableV6/XdgPopupTest.grabbed_popup_gets_keyboard_focus/0
  XdgPopupStable/XdgPopupTest.grabbed_popup_gets_keyboard_focus/0
  LayerShellPopup/XdgPopupTest.grabbed_popup_gets_keyboard_focus/0

  # Not yet implemented, see https://github.com/MirServer/mir/issues/2861
  XdgPopupTest.when_parent_surface_is_moved_a_reactive_popup_is_moved

  # These tests depend on us not sending a configure event when the latest buffer was
  # the "new" size. Even if we have sent a configure event with a different size
  Anchor/LayerSurfaceLayoutTest.maximized_xdg_toplevel_is_shrunk_for_exclusive_zone/0
  Anchor/LayerSurfaceLayoutTest.maximized_xdg_toplevel_is_shrunk_for_exclusive_zone/1
  Anchor/LayerSurfaceLayoutTest.maximized_xdg_toplevel_is_shrunk_for_exclusive_zone/2
  Anchor/LayerSurfaceLayoutTest.maximized_xdg_toplevel_is_shrunk_for_exclusive_zone/3
  Anchor/LayerSurfaceLayoutTest.maximized_xdg_toplevel_is_shrunk_for_exclusive_zone/4
  Anchor/LayerSurfaceLayoutTest.maximized_xdg_toplevel_is_shrunk_for_exclusive_zone/5
  Anchor/LayerSurfaceLayoutTest.maximized_xdg_toplevel_is_shrunk_for_exclusive_zone/6
  Anchor/LayerSurfaceLayoutTest.maximized_xdg_toplevel_is_shrunk_for_exclusive_zone/7
  Anchor/LayerSurfaceLayoutTest.maximized_xdg_toplevel_is_shrunk_for_exclusive_zone/8
  Anchor/LayerSurfaceLayoutTest.maximized_xdg_toplevel_is_shrunk_for_exclusive_zone/9
  Anchor/LayerSurfaceLayoutTest.maximized_xdg_toplevel_is_shrunk_for_exclusive_zone/10
  Anchor/LayerSurfaceLayoutTest.maximized_xdg_toplevel_is_shrunk_for_exclusive_zone/11
  Anchor/LayerSurfaceLayoutTest.maximized_xdg_toplevel_is_shrunk_for_exclusive_zone/12
  Anchor/LayerSurfaceLayoutTest.maximized_xdg_toplevel_is_shrunk_for_exclusive_zone/13
  Anchor/LayerSurfaceLayoutTest.maximized_xdg_toplevel_is_shrunk_for_exclusive_zone/14
  Anchor/LayerSurfaceLayoutTest.maximized_xdg_toplevel_is_shrunk_for_exclusive_zone/15
  Anchor/LayerSurfaceLayoutTest.maximized_xdg_toplevel_is_shrunk_for_exclusive_zone/16
  Anchor/LayerSurfaceLayoutTest.maximized_xdg_toplevel_is_shrunk_for_exclusive_zone/17
  Anchor/LayerSurfaceLayoutTest.maximized_xdg_toplevel_is_shrunk_for_exclusive_zone/18
  Anchor/LayerSurfaceLayoutTest.maximized_xdg_toplevel_is_shrunk_for_exclusive_zone/19
  Anchor/LayerSurfaceLayoutTest.maximized_xdg_toplevel_is_shrunk_for_exclusive_zone/20
  Anchor/LayerSurfaceLayoutTest.maximized_xdg_toplevel_is_shrunk_for_exclusive_zone/21
  Anchor/LayerSurfaceLayoutTest.maximized_xdg_toplevel_is_shrunk_for_exclusive_zone/22
  Anchor/LayerSurfaceLayoutTest.maximized_xdg_toplevel_is_shrunk_for_exclusive_zone/23
  Anchor/LayerSurfaceLayoutTest.maximized_xdg_toplevel_is_shrunk_for_exclusive_zone/24
  Anchor/LayerSurfaceLayoutTest.maximized_xdg_toplevel_is_shrunk_for_exclusive_zone/25
  Anchor/LayerSurfaceLayoutTest.maximized_xdg_toplevel_is_shrunk_for_exclusive_zone/26
  Anchor/LayerSurfaceLayoutTest.maximized_xdg_toplevel_is_shrunk_for_exclusive_zone/27
  Anchor/LayerSurfaceLayoutTest.maximized_xdg_toplevel_is_shrunk_for_exclusive_zone/28
  Anchor/LayerSurfaceLayoutTest.maximized_xdg_toplevel_is_shrunk_for_exclusive_zone/29
  Anchor/LayerSurfaceLayoutTest.maximized_xdg_toplevel_is_shrunk_for_exclusive_zone/30
  Anchor/LayerSurfaceLayoutTest.maximized_xdg_toplevel_is_shrunk_for_exclusive_zone/31
)

if (MIR_SIGBUS_HANDLER_ENVIRONMENT_BROKEN)
  list(APPEND EXPECTED_FAILURES BadBufferTest.test_truncated_shm_file)
endif ()

set(BROKEN_TESTS
  ClientSurfaceEventsTest.buffer_release # Has been removed upstream

  # See https://github.com/MirServer/wlcs/issues/293
  # (Tests pass with https://github.com/MirServer/wlcs/pull/294)
  VirtualPointerV1Test.*
)

if (MIR_RUN_WLCS_TESTS)
  mir_discover_external_gtests(
    NAME wlcs
    COMMAND ${WLCS_BINARY}
    ARGS ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/miral_wlcs_integration.so
    EXPECTED_FAILURES ${EXPECTED_FAILURES}
    BROKEN_TESTS ${BROKEN_TESTS}
  )
endif()

install(TARGETS miral_wlcs_integration LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/mir)

add_custom_target(
  list_missing_wlcs_xfail_tests

  COMMAND
    ${CMAKE_COMMAND}
      -DWLCS_BINARY=${WLCS_BINARY}
      -DWLCS_INTEGRATION=${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/miral_wlcs_integration.so
      -DTESTS_TO_QUERY="${EXPECTED_FAILURES}"
      -P ${CMAKE_CURRENT_SOURCE_DIR}/print_missing_wlcs_tests.cmake
)
