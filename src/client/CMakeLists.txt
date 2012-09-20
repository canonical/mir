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

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 -Werror -Wall -fno-strict-aliasing -pedantic -Wextra")

IF(PROTOBUF_FOUND)

INCLUDE_DIRECTORIES(${PROTOBUF_INCLUDE_DIRS})

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})

ENDIF()

set(
  CLIENT_SOURCES

  mir_client_library.cpp
  android_client_buffer.cpp
  android_registrar.cpp
  mir_rpc_channel.cpp
  ${PROTO_HDRS}
)

add_library( 
  mirclient STATIC
  
  ${CLIENT_SOURCES}
)

add_executable(
  demo_client
  
  demo_client.c
)

target_link_libraries(
  demo_client
  
  mirclient
  mirprotobuf
 
  3rd_party
  ${Boost_LIBRARIES}
  ${PROTOBUF_LIBRARIES}
  ${CMAKE_THREAD_LIBS_INIT}
)

