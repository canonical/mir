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

set(MIRCLIENT_ABI 2)

set_target_properties(
  mirclient

  PROPERTIES
  SOVERSION ${MIRCLIENT_ABI}
)

#
# The version of libEGL.so.N installed on the build machine will often link
# to an older version of libmirclient.so.N than we're trying to build. But if
# we allow EGL clients to link to both then they will depend on TWO different
# sonames of mirclient and get linkage errors. This hack ensures that all EGL
# mir clients in our source tree will only ever link to one version of
# mirclient; that is the new one here.
# It might sound nasty, but it's much nicer than having to maintain mirclient
# and mirserver in different source packages.
#
add_custom_target(mirclient_compat_links
  COMMAND ln -fs libmirclient.so.1 libmirclient.so.0
  WORKING_DIRECTORY ${LIBRARY_OUTPUT_PATH}
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
