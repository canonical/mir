set(PREFIX "${CMAKE_INSTALL_PREFIX}")
set(EXEC_PREFIX "${CMAKE_INSTALL_PREFIX}")
set(LIBDIR "${CMAKE_INSTALL_FULL_LIBDIR}")
set(INCLUDEDIR "${CMAKE_INSTALL_PREFIX}/include/mirclient")

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/mirclient.pc.in
  ${CMAKE_CURRENT_BINARY_DIR}/mirclient.pc
)

include_directories(
  ${PROTOBUF_INCLUDE_DIRS}
  ${CMAKE_CURRENT_BINARY_DIR}
  ${PROJECT_SOURCE_DIR}/include/client
  ${DRM_INCLUDE_DIRS}
)

add_subdirectory(rpc/)
add_subdirectory(lttng/)

if(MIR_BUILD_PLATFORM_ANDROID)
add_subdirectory(android/)
endif()

if(MIR_BUILD_PLATFORM_MESA)
add_subdirectory(mesa/)
endif()

set(
  CLIENT_SOURCES

  aging_buffer.cpp
  client_buffer_depository.cpp
  display_configuration.cpp
  mir_client_library.cpp
  mir_connection.cpp
  mir_wait_handle.cpp
  mir_surface.cpp
  logging/rpc_report.cpp
  logging/input_receiver_report.cpp
  default_connection_configuration.cpp
  surface_map.cpp
  lifecycle_control.cpp
  private.cpp
  mir_screencast.cpp
  mir_screencast_api.cpp
)

add_library(
  mirclient SHARED

  ${CLIENT_SOURCES}
)

set(MIRCLIENT_ABI 7)

set_target_properties(
  mirclient

  PROPERTIES
  SOVERSION ${MIRCLIENT_ABI}
)

set(
  MIR_CLIENT_LIBRARIES
  ${Boost_LIBRARIES}
  ${CMAKE_THREAD_LIBS_INIT}
  ${PROTOBUF_LIBRARIES}
  ${XKBCOMMON_LIBRARIES}

  mirprotobuf
  mirsharedinput
  mirsharedlogging
  mirsharedenv
  mirsharedsharedlibrary
  mirclientrpc
  mirclientlttngstatic

  ${MIR_COMMON_PLATFORM_LIBRARIES}
 
  3rd_party
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
  DIRECTORY ${CMAKE_SOURCE_DIR}/include/client/mir_toolkit ${CMAKE_SOURCE_DIR}/include/client/mir
  DESTINATION "include/mirclient"
)

install(
  FILES ${CMAKE_CURRENT_BINARY_DIR}/mirclient.pc
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)
