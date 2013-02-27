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
# Authored by: Thomas Voss <thomas.voss@canonical.com>,
#              Alan Griffiths <alan@octopull.co.uk>

set(
  MIR_COMPOSITOR_SRCS

  compositor.cpp
  buffer_basic.cpp
  temporary_buffers.cpp
  buffer_bundle_manager.cpp
  buffer_bundle_surfaces.cpp
  buffer_swapper_multi.cpp
  swapper_factory.cpp
  rendering_operator.cpp
)

ADD_LIBRARY(
  mircompositor STATIC

  ${MIR_COMPOSITOR_SRCS}
)

target_link_libraries(mircompositor
  ${Boost_LIBRARIES}
)

