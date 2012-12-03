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
  session.cpp
  session_container.cpp
  session_manager.cpp
  registration_order_focus_sequence.cpp
  single_visibility_focus_mechanism.cpp
  null_application_listener.cpp
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
  binder_session.cpp
)
endif()

add_library( 
  mirfrontend STATIC
  
  ${FRONTEND_SOURCES}
)

target_link_libraries(
  mirfrontend

  mirlogging
  3rd_party
)
