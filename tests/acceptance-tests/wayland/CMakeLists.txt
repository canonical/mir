include_directories(${WLCS_INCLUDE_DIRS})

add_library(
  miral_wlcs_integration MODULE

  miral_integration.cpp
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

set(GTEST_FILTER "--gtest_filter=-\
ClientSurfaceEventsTest.frame_timestamp_increases:\
ClientSurfaceEventsTest.surface_gets_enter_event:\
ClientSurfaceEventsTest.surface_gets_leave_event:\
SubsurfaceTest.place_below_simple:\
SubsurfaceTest.place_above_simple:\
XdgSurfaceStableTest.attaching_buffer_to_unconfigured_xdg_surface_is_an_error:\
XdgSurfaceStableTest.creating_xdg_surface_from_wl_surface_with_committed_buffer_is_an_error:\
XdgSurfaceStableTest.creating_xdg_surface_from_wl_surface_with_attached_buffer_is_an_error:\
XdgSurfaceStableTest.creating_xdg_surface_from_wl_surface_with_existing_role_is_an_error")

mir_discover_external_gtests(
  NAME wlcs
  COMMAND ${WLCS_BINARY} ${GTEST_FILTER} ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/miral_wlcs_integration.so
)

install(TARGETS miral_wlcs_integration LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/mir)
