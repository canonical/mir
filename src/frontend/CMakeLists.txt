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

  application.cpp
  application_proxy.cpp
  application_session.cpp
  application_session_model.cpp
  registration_order_focus_strategy.cpp
  null_application_listener.cpp
  application_manager.cpp
  protobuf_asio_communicator.cpp

  resource_cache.cpp
  ${PROTO_HDRS}
)

add_library( 
  mirfrontend STATIC
  
  ${FRONTEND_SOURCES}
)

target_link_libraries(
  mirfrontend

  mirlogging
)
