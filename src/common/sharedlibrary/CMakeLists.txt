# Copyright © 2014 Canonical Ltd.
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
#
# Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>

add_library(mirsharedsharedlibrary OBJECT
  module_deleter.cpp
  shared_library.cpp
  shared_library_prober.cpp
)

# we overload dlvsym() for systems that don't have it
set_source_files_properties(shared_library.cpp
  PROPERTIES COMPILE_FLAGS "-Wno-unused-function"
)

list(APPEND MIR_COMMON_SOURCES
  $<TARGET_OBJECTS:mirsharedsharedlibrary>
)

list(APPEND MIR_COMMON_REFERENCES 
  ${Boost_LIBRARIES}
  dl
)

set(MIR_COMMON_SOURCES ${MIR_COMMON_SOURCES} PARENT_SCOPE)
set(MIR_COMMON_REFERENCES ${MIR_COMMON_REFERENCES} PARENT_SCOPE)
