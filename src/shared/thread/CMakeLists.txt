# Copyright © 2014 Canonical Ltd.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License version 3 as
# published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
# Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>

add_library(mirsharedthread STATIC
  thread_name.cpp
  recursive_read_write_mutex.cpp
)

target_link_libraries(
    mirsharedthread
)

list(APPEND MIR_COMMON_LIBRARIES
  mirsharedthread
)

set(MIR_COMMON_LIBRARIES ${MIR_COMMON_LIBRARIES} PARENT_SCOPE)