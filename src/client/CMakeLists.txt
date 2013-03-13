set(MIR_CLIENT_INCLUDE_DIR_SUFFIX "mir/client/")

set(PREFIX "${CMAKE_INSTALL_PREFIX}")
set(EXEC_PREFIX "${CMAKE_INSTALL_PREFIX}")
set(LIBDIR "${CMAKE_INSTALL_PREFIX}/lib")
set(INCLUDEDIR "${CMAKE_INSTALL_PREFIX}/include/${MIR_CLIENT_INCLUDE_DIR_SUFFIX}")

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/mirclient.pc.in
  ${CMAKE_CURRENT_BINARY_DIR}/mirclient.pc
)

if(PROTOBUF_FOUND)

include_directories(
  ${PROTOBUF_INCLUDE_DIRS}
  ${CMAKE_CURRENT_BINARY_DIR}
)

endif()

include_directories(
  ${PROJECT_SOURCE_DIR}/include/client
  ${DRM_INCLUDE_DIRS}
)

set(
  CLIENT_SOURCES

  mir_client_library.cpp
  mir_connection.cpp
  mir_wait_handle.cpp
  mir_surface.cpp
  mir_logger.cpp
  mir_basic_rpc_channel.cpp
  ${PROTO_SRCS}
)

if (MIR_TRANSPORT STREQUAL "socket")
list(APPEND CLIENT_SOURCES
  mir_socket_rpc_channel.cpp
  make_socket_rpc_channel.cpp
)
elseif (MIR_TRANSPORT STREQUAL "binder")
list(APPEND CLIENT_SOURCES
  mir_binder_rpc_channel.cpp
  make_binder_rpc_channel.cpp
)
endif()

if( MIR_PLATFORM STREQUAL "android")
include_directories( ${LIBHARDWARE_INCLUDE_DIRS} ${ANDROID_UI_INCLUDE_DIRS})
set(
  CLIENT_SOURCES

  android/android_client_buffer.cpp
  android/android_client_buffer_depository.cpp
  android/android_registrar.cpp
  android/android_client_platform.cpp
  android/mir_native_window.cpp
  android/client_surface_interpreter.cpp
  ${CLIENT_SOURCES}
)
endif()

if( MIR_PLATFORM STREQUAL "gbm")
set(
  CLIENT_SOURCES

  gbm/gbm_client_platform.cpp
  gbm/gbm_client_buffer_depository.cpp
  gbm/gbm_client_buffer.cpp
  ${CLIENT_SOURCES}
)
endif()

add_library(
  mirclient SHARED

  ${CLIENT_SOURCES}
  ${MIR_3RD_PARTY_SOURCES}
)

set_target_properties(
  mirclient

  PROPERTIES
  VERSION ${MIR_VERSION_MAJOR}.${MIR_VERSION_MINOR}.${MIR_VERSION_PATCH}
  SOVERSION ${MIR_VERSION_MAJOR}
)

target_link_libraries(
  mirclient

  ${Boost_LIBRARIES}
  ${CMAKE_THREAD_LIBS_INIT}
  ${LIBHARDWARE_LIBRARIES}
  ${PROTOBUF_LIBRARIES}

  mirprotobuf

  ${DRM_LIBRARIES}
  ${ANDROID_STDLIB}
  3rd_party
)

set(
  CLIENT_PUBLIC_HEADERS

  ${CMAKE_SOURCE_DIR}/include/client/mir_toolkit/mir_client_library.h
  ${CMAKE_SOURCE_DIR}/include/client/mir_toolkit/mir_client_library_lightdm.h
  ${CMAKE_SOURCE_DIR}/include/client/mir_toolkit/mir_client_library_drm.h
)

set_target_properties(
  mirclient
  PROPERTIES
  PUBLIC_HEADER "${CLIENT_PUBLIC_HEADERS}"
)

install(
  TARGETS mirclient
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  PUBLIC_HEADER DESTINATION "include/${MIR_CLIENT_INCLUDE_DIR_SUFFIX}")

install(
  FILES ${CMAKE_CURRENT_BINARY_DIR}/mirclient.pc
  DESTINATION lib/pkgconfig
)
