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


set(
  FRONTEND_SOURCES

  application_mediator.cpp
  null_application_mediator_report.cpp
  protobuf_message_processor.cpp
  null_message_processor.cpp

  resource_cache.cpp
  ${PROTO_HDRS}
)

if (MIR_TRANSPORT STREQUAL "socket")
list(APPEND FRONTEND_SOURCES
  protobuf_socket_communicator.cpp
  make_protobuf_socket_communicator.cpp
  socket_session.cpp
)
elseif (MIR_TRANSPORT STREQUAL "binder")
list(APPEND FRONTEND_SOURCES
  protobuf_binder_communicator.cpp
  make_protobuf_binder_communicator.cpp
  binder_service.cpp
)
endif()

if (MIR_PLATFORM STREQUAL "android")
  list(APPEND FRONTEND_SOURCES application_mediator_android.cpp)
elseif (MIR_PLATFORM STREQUAL "gbm")
  include_directories(${DRM_INCLUDE_DIRS})
  list(APPEND FRONTEND_SOURCES application_mediator_gbm.cpp)
endif()

# TODO replace this frig with an RTTI enabled binder library
if (MIR_TRANSPORT STREQUAL "binder")
set_source_files_properties(
  protobuf_binder_communicator.cpp
  binder_service.cpp
  PROPERTIES
  COMPILE_FLAGS "-fno-rtti")
endif()

add_library(
  mirfrontend STATIC

  ${FRONTEND_SOURCES}
)

# Fix build race condition - ensure the protobuf header is built before us.
add_dependencies(mirfrontend mirprotobuf)

target_link_libraries(
  mirfrontend

  mirsessions
  mirlogging
  3rd_party
  ${Boost_LIBRARIES}
)
