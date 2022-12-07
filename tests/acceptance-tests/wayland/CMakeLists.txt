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
  ${WAYLAND_CLIENT_LDFLAGS}
)

set(SYMBOL_FILE ${CMAKE_CURRENT_SOURCE_DIR}/symbol.map)

set_target_properties(
  miral_wlcs_integration PROPERTIES

  PREFIX ""
  LINK_FLAGS "-Wl,--version-script,${SYMBOL_FILE}"
  LINK_DEPENDS ${SYMBOL_FILE}
)

pkg_get_variable(WLCS_BINARY wlcs test_runner)

set(EXPECTED_FAILURES

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

  AllSurfaceTypes/TouchTest.sends_touch_up_on_surface_destroy/4 # Fixed by https://github.com/MirServer/wlcs/pull/199
  AllSurfaceTypes/TouchTest.sends_touch_up_on_surface_destroy/5 # Fixed by https://github.com/MirServer/wlcs/pull/199

  ForeignToplevelHandleTest.can_maximize_foreign_while_fullscreen # https://github.com/MirServer/mir/issues/2164

  # See https://github.com/MirServer/mir/issues/2324
  TextInputV3WithInputMethodV2Test.text_input_enters_parent_surface_after_child_destroyed
  TextInputV3WithInputMethodV2Test.text_input_enters_grabbing_popup
  XdgPopupUnstableV6/XdgPopupTest.grabbed_popup_gets_keyboard_focus/0
  XdgPopupStable/XdgPopupTest.grabbed_popup_gets_keyboard_focus/0
  LayerShellPopup/XdgPopupTest.grabbed_popup_gets_keyboard_focus/0
)

if (MIR_BAD_BUFFER_TEST_ENVIRONMENT_BROKEN)
  list(APPEND EXPECTED_FAILURES BadBufferTest.test_truncated_shm_file)
endif ()

set(BROKEN_TESTS
  ClientSurfaceEventsTest.buffer_release # Has been removed upstream
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
