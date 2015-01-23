set(PREFIX "${CMAKE_INSTALL_PREFIX}")
set(EXEC_PREFIX "${CMAKE_INSTALL_PREFIX}")
set(LIBDIR "${CMAKE_INSTALL_FULL_LIBDIR}")
set(INCLUDEDIR "${CMAKE_INSTALL_PREFIX}/include")

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/mirclient.pc.in
  ${CMAKE_CURRENT_BINARY_DIR}/mirclient.pc
  @ONLY
)

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/mirclient-debug-extension.pc.in
  ${CMAKE_CURRENT_BINARY_DIR}/mirclient-debug-extension.pc
  @ONLY
)

include_directories(
  ${PROTOBUF_INCLUDE_DIRS}
  ${CMAKE_CURRENT_BINARY_DIR}
  ${PROJECT_SOURCE_DIR}/include/client
  ${PROJECT_SOURCE_DIR}/src/include/client
  ${DRM_INCLUDE_DIRS}
)

set(MIRCLIENT_ABI 8)
set(CLIENT_PLATFORM_VERSION "MIR_CLIENT_PLATFORM_2")
set(symbol_map ${CMAKE_SOURCE_DIR}/src/client/symbols.map)

add_definitions(-DMIR_CLIENT_PLATFORM_PATH="${MIR_CLIENT_PLATFORM_PATH}/")
add_definitions(-DCLIENT_PLATFORM_VERSION="${CLIENT_PLATFORM_VERSION}")
add_definitions(-DMIR_LOGGING_COMPONENT_FALLBACK="mirclient")

add_subdirectory(rpc/)
add_subdirectory(lttng/)

add_library(mirclientobjects OBJECT

  client_buffer_depository.cpp
  display_configuration.cpp
  error_connections.cpp
  mir_connection.cpp
  mir_connection_api.cpp
  mir_wait_handle.cpp
  mir_wait_api.cpp
  mir_surface.cpp
  mir_surface_api.cpp
  logging/rpc_report.cpp
  logging/input_receiver_report.cpp
  logging/perf_report.cpp
  default_connection_configuration.cpp
  surface_map.cpp
  lifecycle_control.cpp
  private.cpp
  mir_screencast.cpp
  mir_screencast_api.cpp
  mir_cursor_api.cpp
  mir_prompt_session.cpp
  mir_prompt_session_api.cpp
  mir_event_distributor.cpp
  probing_client_platform_factory.cpp
  periodic_perf_report.cpp
  mir_platform_message_api.cpp
)

# Ensure protobuf C++ headers have been produced before
# trying to compile mirclientobjects
add_dependencies(mirclientobjects mirprotobuf)

add_library(mirclient SHARED

  $<TARGET_OBJECTS:mirclientobjects>
)

set_target_properties(mirclient

  PROPERTIES
  SOVERSION ${MIRCLIENT_ABI}
  LINK_FLAGS "-Wl,--exclude-libs=ALL -Wl,--version-script,${symbol_map}"
)

target_link_libraries(mirclient
  mirclientrpc
  mirclientlttngstatic
  mircommon
  mirprotobuf

  ${Boost_LIBRARIES}
  ${CMAKE_THREAD_LIBS_INIT}
  ${XKBCOMMON_LIBRARIES}
)

install(
  TARGETS mirclient
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

install(
  DIRECTORY ${CMAKE_SOURCE_DIR}/include/client/mir_toolkit
  DESTINATION "include/mirclient"
)

install(
  FILES ${CMAKE_CURRENT_BINARY_DIR}/mirclient.pc
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)

# TODO: Extension apparatus so that mirclient-debug-extension can be a MODULE and not
#       duplicate (almost) the entirity of mirclient

set(MIRCLIENT_DEBUG_EXTENSION_ABI 1)

add_library(
  mirclient-debug-extension SHARED

  mir_debug_api.cpp
  $<TARGET_OBJECTS:mirclientobjects>
)

target_link_libraries(
  mirclient-debug-extension

  mirclient
)

set(debug_symbol_map ${CMAKE_CURRENT_SOURCE_DIR}/symbols-debug.map)

set_target_properties(
  mirclient-debug-extension

  PROPERTIES
  SOVERSION ${MIRCLIENT_DEBUG_EXTENSION_ABI}
  LINK_FLAGS "-Wl,--version-script,${debug_symbol_map}"
)

install(
  DIRECTORY ${CMAKE_SOURCE_DIR}/include/client/mir_toolkit/debug
  DESTINATION "include/mirclient/mir_toolkit/debug"
)

install(
  TARGETS mirclient-debug-extension
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

install(
  FILES ${CMAKE_CURRENT_BINARY_DIR}/mirclient-debug-extension.pc
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)

