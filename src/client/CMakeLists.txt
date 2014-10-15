set(MIRCLIENT_ABI 8)
set(CLIENT_DRIVER client${MIRCLIENT_ABI}driver)
set(MIR_CLIENT_DRIVER mir${CLIENT_DRIVER})
set(MIR_CLIENT_DRIVER_BINARY lib${MIR_CLIENT_DRIVER}.so
    CACHE STRING "File name of Mir client drivers")
add_definitions(-DMIR_CLIENT_DRIVER_BINARY="${MIR_CLIENT_DRIVER_BINARY}")

set(PREFIX "${CMAKE_INSTALL_PREFIX}")
set(EXEC_PREFIX "${CMAKE_INSTALL_PREFIX}")
set(LIBDIR "${CMAKE_INSTALL_FULL_LIBDIR}")
set(INCLUDEDIR "${CMAKE_INSTALL_PREFIX}/include/mirclient")
set(COMMON_INCLUDEDIR "${CMAKE_INSTALL_PREFIX}/include/mircommon")

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/mirclient.pc.in
  ${CMAKE_CURRENT_BINARY_DIR}/mirclient.pc
)

include_directories(
  ${PROTOBUF_INCLUDE_DIRS}
  ${CMAKE_CURRENT_BINARY_DIR}
  ${PROJECT_SOURCE_DIR}/include/client
  ${PROJECT_SOURCE_DIR}/src/include/client
  ${DRM_INCLUDE_DIRS}
)

add_subdirectory(rpc/)
add_subdirectory(lttng/)

add_library(mirclient_platformimpl OBJECT
  aging_buffer.cpp
)

if(MIR_BUILD_PLATFORM_ANDROID)
add_subdirectory(android/)
endif()

if(MIR_BUILD_PLATFORM_MESA)
add_subdirectory(mesa/)
endif()

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
  periodic_perf_report.cpp
)

add_library(
  mirclient SHARED

  $<TARGET_OBJECTS:mirclientobjects>
)

set(symbol_map ${CMAKE_CURRENT_SOURCE_DIR}/symbols.map)

set_target_properties(
  mirclient

  PROPERTIES
  SOVERSION ${MIRCLIENT_ABI}
  LINK_FLAGS "-Wl,--exclude-libs=ALL -Wl,--version-script,${symbol_map}"
)

set(
  MIR_CLIENT_LIBRARIES
  mirclientrpc
  mirclientlttngstatic
  mircommon

  ${Boost_LIBRARIES}
  ${CMAKE_THREAD_LIBS_INIT}
  ${XKBCOMMON_LIBRARIES}
)

target_link_libraries(
  mirclient

  ${MIR_CLIENT_LIBRARIES}
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
