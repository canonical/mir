# Copyright © Canonical Ltd.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License version 2 or 3 as
# published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

add_library(mircommondispatch OBJECT
  action_queue.cpp
  multiplexing_dispatchable.cpp
  readable_fd.cpp
  threaded_dispatcher.cpp
  utils.cpp
  signal_blocker.cpp
)

target_include_directories(mircommondispatch
  PUBLIC
    ${PROJECT_SOURCE_DIR}/include/common
)

target_link_libraries(mircommondispatch
  PUBLIC
    mircore
)
