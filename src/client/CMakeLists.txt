# Copyright © 2012 Canonical Ltd.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 3 as
# published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
# Authored by: Alan Griffiths <alan@octopull.co.uk>

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
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${DRM_INCLUDE_DIRS}
)

set(
  CLIENT_SOURCES

  client_buffer_depository.cpp
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
  android/android_client_buffer_factory.cpp
  android/android_registrar.cpp
  android/android_client_platform.cpp
  android/mir_native_window.cpp
  ${CLIENT_SOURCES}
)
endif()

if( MIR_PLATFORM STREQUAL "gbm")
set(
  CLIENT_SOURCES

  gbm/gbm_client_platform.cpp
  gbm/gbm_client_buffer_factory.cpp
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

  ${CMAKE_SOURCE_DIR}/include/mir_toolkit/mir_client_library.h
  ${CMAKE_SOURCE_DIR}/include/mir_toolkit/mir_client_library_lightdm.h
  ${CMAKE_SOURCE_DIR}/include/mir_toolkit/mir_client_library_drm.h
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
