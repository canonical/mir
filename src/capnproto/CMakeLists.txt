# Copyright © 2016 Canonical Ltd.
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU Lesser General Public License version 3,
# as published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
# Author: Brandon Schaefer <brandon.schaefer@canonical.com>

set(CAPNPC_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR})

capnp_generate_cpp(CAPNPROTO_SRC CAPNPROTO_HDRS mir_event.capnp)

add_library(mircapnproto STATIC ${CAPNPROTO_SRC})
target_link_libraries(mircapnproto ${CAPNP_LIBRARIES_LITE})

list(APPEND MIR_GENERATED_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_BINARY_DIR})

set(
  MIR_GENERATED_INCLUDE_DIRECTORIES ${MIR_GENERATED_INCLUDE_DIRECTORIES}
  PARENT_SCOPE
)
