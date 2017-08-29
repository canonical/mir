find_package(XKBCOMMON REQUIRED)

set(PREFIX "${CMAKE_INSTALL_PREFIX}")
set(EXEC_PREFIX "${CMAKE_INSTALL_PREFIX}")
set(LIBDIR "${CMAKE_INSTALL_FULL_LIBDIR}")
set(INCLUDEDIR "${CMAKE_INSTALL_PREFIX}/include")

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/mirclient.pc.in
  ${CMAKE_CURRENT_BINARY_DIR}/mirclient.pc
  @ONLY
)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/mirclientcpp.pc.in
  ${CMAKE_CURRENT_BINARY_DIR}/mirclientcpp.pc
  @ONLY
)

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/mirclient-debug-extension.pc.in
  ${CMAKE_CURRENT_BINARY_DIR}/mirclient-debug-extension.pc
  @ONLY
)

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/version.h.in
  ${PROJECT_SOURCE_DIR}/include/client/mir_toolkit/version.h
)

include_directories(
  ${PROTOBUF_INCLUDE_DIRS}
  ${CMAKE_CURRENT_BINARY_DIR}
  ${PROJECT_SOURCE_DIR}/include/platform
  ${PROJECT_SOURCE_DIR}/include/client
  ${PROJECT_SOURCE_DIR}/src/include/client
  ${PROJECT_SOURCE_DIR}/src/include/cookie
  ${DRM_INCLUDE_DIRS}
)

set(MIRCLIENT_ABI 9)
set(symbol_map ${CMAKE_SOURCE_DIR}/src/client/symbols.map)

add_definitions(-DMIR_CLIENT_PLATFORM_PATH="${MIR_CLIENT_PLATFORM_PATH}/")
add_definitions(-DCLIENT_PLATFORM_VERSION="${MIR_CLIENT_PLATFORM_VERSION}")
add_definitions(-DMIR_LOG_COMPONENT_FALLBACK="mirclient")

set(MIR_CLIENT_SOURCES)
set(MIR_CLIENT_REFERENCES mircommon)

add_subdirectory(rpc/)
add_subdirectory(lttng/)
add_subdirectory(events)
add_subdirectory(input)

add_library(mirclientobjects OBJECT

  display_configuration.cpp
  drag_and_drop.cpp drag_and_drop.h
  error_connections.cpp
  event.cpp
  event_printer.cpp
  mir_blob.cpp
  mir_cookie.cpp
  mir_connection.cpp
  mir_connection_api.cpp
  mir_input_device_api.cpp
  mir_wait_handle.cpp
  mir_wait_api.cpp
  mir_surface.cpp
  mir_surface_api.cpp
  logging/rpc_report.cpp
  logging/input_receiver_report.cpp
  logging/perf_report.cpp
  default_connection_configuration.cpp
  connection_surface_map.cpp
  frame_clock.cpp
  mir_screencast.cpp
  mir_screencast_api.cpp
  mir_cursor_api.cpp
  mir_prompt_session.cpp
  mir_prompt_session_api.cpp
  mir_event_distributor.cpp
  probing_client_platform_factory.cpp
  periodic_perf_report.cpp
  mir_platform_message_api.cpp
  buffer_stream.cpp
  screencast_stream.cpp
  buffer_vault.cpp
  mir_buffer_stream_api.cpp
  error_stream.cpp
  error_render_surface.cpp
  buffer.cpp
  error_buffer.cpp
  mir_render_surface_api.cpp
  render_surface.cpp
  presentation_chain.cpp
  mir_presentation_chain_api.cpp
  mir_buffer_api.cpp
  display_configuration_api.cpp
  protobuf_to_native_buffer.cpp
  buffer_factory.cpp
  mir_error.cpp
  mir_error.h
  mir_error_api.cpp
  buffer_stream_configuration.cpp
  error_handler.h
  ${MIR_CLIENT_SOURCES}
  ${CMAKE_SOURCE_DIR}/include/client/mir_toolkit/events/surface_output_event.h
  ${CMAKE_SOURCE_DIR}/include/client/mir_toolkit/mir_display_configuration.h
  ${CMAKE_SOURCE_DIR}/include/client/mir_toolkit/mir_error.h
  mir_extension_core.cpp
  ${CMAKE_SOURCE_DIR}/include/client/mir_toolkit/mir_extension_core.h
  ${CMAKE_SOURCE_DIR}/include/client/mir_toolkit/extensions/drag_and_drop.h
  ${CMAKE_SOURCE_DIR}/include/client/mir/client/blob.h
  ${CMAKE_SOURCE_DIR}/include/client/mir/client/cookie.h
  ${CMAKE_SOURCE_DIR}/include/client/mir/client/window_spec.h
  ${CMAKE_SOURCE_DIR}/include/client/mir/client/window_id.h
  ${CMAKE_SOURCE_DIR}/include/client/mir/client/connection.h
  ${CMAKE_SOURCE_DIR}/include/client/mir/client/display_config.h
  ${CMAKE_SOURCE_DIR}/include/client/mir/client/window.h
)

# Ensure protobuf C++ headers have been produced before
# trying to compile mirclientobjects
add_dependencies(mirclientobjects mirprotobuf)
add_dependencies(mirclientobjects mircapnproto)

add_library(mirclient SHARED $<TARGET_OBJECTS:mirclientobjects> $<TARGET_OBJECTS:mirsharedinput> $<TARGET_OBJECTS:mirclientrpc>)
add_library(mirclient-static STATIC $<TARGET_OBJECTS:mirclientobjects> $<TARGET_OBJECTS:mirsharedinput> $<TARGET_OBJECTS:mirclientrpc>)

set_target_properties(mirclient

  PROPERTIES
  SOVERSION ${MIRCLIENT_ABI}
  LINK_FLAGS "-Wl,--exclude-libs=ALL -Wl,--version-script,${symbol_map}"
  LINK_DEPENDS ${symbol_map}
)

set(LINKS
  mircommon
  mirprotobuf
  mircookie

  ${XKBCOMMON_LIBRARIES}
)

target_link_libraries(mirclient
  LINK_PRIVATE
  mirclientlttng-static
  LINK_PUBLIC
  ${MIR_CLIENT_REFERENCES}
  ${LINKS}
)

install(
  TARGETS mirclient
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

install(
  DIRECTORY ${CMAKE_SOURCE_DIR}/include/client/mir_toolkit
            ${CMAKE_SOURCE_DIR}/include/client/mir
  DESTINATION "include/mirclient"
)

install(
  FILES ${CMAKE_CURRENT_BINARY_DIR}/mirclient.pc
        ${CMAKE_CURRENT_BINARY_DIR}/mirclientcpp.pc
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)

set(MIRCLIENT_DEBUG_EXTENSION_ABI 1)
set(debug_symbol_map ${CMAKE_CURRENT_SOURCE_DIR}/symbols-debug.map)

add_library(mirclient-debug-extension SHARED
  mir_debug_api.cpp
)

target_link_libraries(mirclient-debug-extension
  PRIVATE
  mirclient-static  # Link statically; only copy what you need.
                    # This allows access to private symbols normally hidden
                    # in the shared version of libmirclient.
  mirclientlttng-static
  PUBLIC
  ${LINKS}
)

set_target_properties(
  mirclient-debug-extension

  PROPERTIES
  SOVERSION ${MIRCLIENT_DEBUG_EXTENSION_ABI}
  LINK_FLAGS "-Wl,--exclude-libs=ALL -Wl,--version-script,${debug_symbol_map}"
  LINK_DEPENDS ${debug_symbol_map}
)

install(
  TARGETS mirclient-debug-extension
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

install(
  FILES ${CMAKE_CURRENT_BINARY_DIR}/mirclient-debug-extension.pc
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)

