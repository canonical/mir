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

include(CheckCXXSymbolExists)

list(APPEND CMAKE_REQUIRED_LIBRARIES dl)
list(APPEND CMAKE_REQUIRED_DEFINITIONS -D_GNU_SOURCE)

check_cxx_symbol_exists("dlvsym" "dlfcn.h" HAS_DLVSYM)
check_cxx_symbol_exists("dlsym" "dlfcn.h" HAS_DLSYM)

if (NOT HAS_DLVSYM)
  if (NOT HAS_DLSYM)
    message(
      FATAL_ERROR
      "Could not detect dlvsym or dlsym"
    )
  endif()

  set_source_files_properties (
    "shared_library.cpp"
    PROPERTIES COMPILE_DEFINITIONS MIR_DONT_USE_DLVSYM="1"
  )

  message(
    WARNING
    "dlvsym() not supported by libc. Mir may attempt to load ABI-incompatible platform modules"
  )
endif()

add_library(mirsharedsharedlibrary OBJECT
  module_deleter.cpp
  shared_library.cpp
  shared_library_prober.cpp
)

target_link_libraries(mirsharedsharedlibrary
  PUBLIC
    Boost::filesystem
    dl
)

list(APPEND MIR_COMMON_REFERENCES
  mirsharedsharedlibrary
)

set(MIR_COMMON_REFERENCES ${MIR_COMMON_REFERENCES} PARENT_SCOPE)
