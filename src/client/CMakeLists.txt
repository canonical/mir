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

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 -Wall -fno-strict-aliasing -Wextra")

if(PROTOBUF_FOUND)

include_directories(
  ${PROTOBUF_INCLUDE_DIRS}
  ${CMAKE_CURRENT_BINARY_DIR}
)

endif()

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
)

set(
  CLIENT_SOURCES

  mir_client_library.cpp
  private/mir_rpc_channel.cpp
  private/mir_connection.cpp
  private/mir_wait_handle.cpp
  private/mir_surface.cpp
  ${PROTO_SRCS}
)

if( MIR_PLATFORM STREQUAL "android")
set(
  CLIENT_SOURCES

  private/android/android_client_buffer.cpp
  private/android/android_registrar.cpp
  ${CLIENT_SOURCES}
)
endif()

add_library( 
  mirclient STATIC
  
  ${CLIENT_SOURCES}
  ${MIR_3RD_PARTY_SOURCES}
)

set_target_properties(
  mirclient 

  PROPERTIES
  VERSION ${MIR_VERSION_MAJOR}.${MIR_VERSION_MINOR}.${MIR_VERSION_PATCH}
  SOVERSION ${MIR_VERSION_MAJOR}.${MIR_VERSION_MINOR}.${MIR_VERSION_PATCH}
)

target_link_libraries(
  mirclient

  mirprotobuf

  ${Boost_LIBRARIES}
  ${PROTOBUF_LIBRARIES}
  ${CMAKE_THREAD_LIBS_INIT}
)

add_executable(
  ubuntu_ds_demo_client
  
  demo_client.c
)

target_link_libraries(
  ubuntu_ds_demo_client
  
  mirclient
  mirprotobuf
  
  ${Boost_LIBRARIES}
  ${PROTOBUF_LIBRARIES}
  ${CMAKE_THREAD_LIBS_INIT}
)

install(
  TARGETS ubuntu_ds_demo_client
  RUNTIME DESTINATION bin)

set_target_properties(
  mirclient
  PROPERTIES 
  PUBLIC_HEADER ${CMAKE_SOURCE_DIR}/include/mir_client/mir_client_library.h
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
