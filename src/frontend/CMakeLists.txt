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
# Authored by: Thomas Voss <thomas.voss@canonical.com>

IF(PROTOBUF_FOUND)

INCLUDE_DIRECTORIES(${PROTOBUF_INCLUDE_DIRS})

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})

PROTOBUF_GENERATE_CPP(PROTO_SRCS PROTO_HDRS
  protobuf/mir_protobuf_wire.proto
  protobuf/mir_protobuf.proto
)

ENDIF()

set(
  FRONTEND_SOURCES

  application.cpp
  application_manager.cpp
  protobuf_asio_communicator.cpp
  ${PROTO_SRCS}
  ${PROTO_HDRS}
)

add_library( 
  mirfrontend STATIC
  
  ${FRONTEND_SOURCES}
)
