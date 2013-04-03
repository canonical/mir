set(PREFIX "${CMAKE_INSTALL_PREFIX}")
set(EXEC_PREFIX "${CMAKE_INSTALL_PREFIX}")
set(LIBDIR "${CMAKE_INSTALL_PREFIX}/lib")
set(INCLUDEDIR "${CMAKE_INSTALL_PREFIX}/include/mirclient")

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

  aging_buffer.cpp
  client_buffer_depository.cpp
  mir_client_library.cpp
  mir_connection.cpp
  mir_wait_handle.cpp
  mir_surface.cpp
  mir_logger.cpp
  mir_basic_rpc_channel.cpp
  ${PROTO_SRCS}
)
add_subdirectory(input)

list(APPEND CLIENT_SOURCES
  mir_socket_rpc_channel.cpp
  make_socket_rpc_channel.cpp
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
  ${CLIENT_SOURCES}
)
endif()

add_library(
  mirclient SHARED

  ${CLIENT_SOURCES}
)

set_target_properties(
  mirclient

  PROPERTIES
  VERSION ${MIR_VERSION_MAJOR}.${MIR_VERSION_MINOR}.${MIR_VERSION_PATCH}
  SOVERSION ${MIR_VERSION_MAJOR}
)

set(
  MIR_CLIENT_LIBRARIES
  ${Boost_LIBRARIES}
  ${CMAKE_THREAD_LIBS_INIT}
  ${LIBHARDWARE_LIBRARIES}
  ${PROTOBUF_LIBRARIES}

  mirprotobuf
  mirinputandroidtransport

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
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)

install(
  DIRECTORY ${CMAKE_SOURCE_DIR}/include/client/mir_toolkit
  DESTINATION "include/mirclient"
)

install(
  FILES ${CMAKE_CURRENT_BINARY_DIR}/mirclient.pc
  DESTINATION lib/pkgconfig
)
