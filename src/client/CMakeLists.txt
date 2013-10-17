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
  default_connection_configuration.cpp
  surface_map.cpp
  lifecycle_control.cpp
)

if( MIR_PLATFORM STREQUAL "android")
include_directories(${LIBHARDWARE_INCLUDE_DIRS})
set(
  CLIENT_SOURCES

  android/android_client_buffer.cpp
  android/android_client_buffer_factory.cpp
  android/android_registrar.cpp
  android/android_client_platform.cpp
  android/client_surface_interpreter.cpp
  android/android_native_display_container.cpp
  ${CLIENT_SOURCES}
)
endif()

if( MIR_PLATFORM STREQUAL "gbm")
set(
  CLIENT_SOURCES

  gbm/gbm_client_platform.cpp
  gbm/gbm_client_buffer_factory.cpp
  gbm/gbm_client_buffer.cpp
  gbm/mesa_native_display_container.cpp
  gbm/gbm_native_surface.cpp
  ${CLIENT_SOURCES}
)
endif()

add_library(
  mirclient SHARED

  ${CLIENT_SOURCES}
)

set(MIRCLIENT_ABI 3)

set_target_properties(
  mirclient

  PROPERTIES
  SOVERSION ${MIRCLIENT_ABI}
)

# Ensure mirclient_compat_links exist before any dependents of mirclient
add_dependencies(mirclient mirclient_compat_links)

set(
  MIR_CLIENT_LIBRARIES
  ${Boost_LIBRARIES}
  ${CMAKE_THREAD_LIBS_INIT}
  ${LIBHARDWARE_LIBRARIES}
  ${PROTOBUF_LIBRARIES}
  ${XKBCOMMON_LIBRARIES}

  mirprotobuf
  mirsharedinput
  mirsharedlogging
  mirclientrpc
  mirclientlttngstatic

  ${DRM_LIBRARIES}
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
  DIRECTORY ${CMAKE_SOURCE_DIR}/include/client/mir_toolkit
  DESTINATION "include/mirclient"
)

install(
  FILES ${CMAKE_CURRENT_BINARY_DIR}/mirclient.pc
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)
