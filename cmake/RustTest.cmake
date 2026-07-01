# Copyright © Canonical Ltd.
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 2 or 3,
# as published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

find_program(CARGO_EXECUTABLE cargo)

# mir_add_cargo_test(NAME <test-name> CRATES <crate>...)
#
# Registers a CTest test that runs `cargo test` for the given workspace
# crate(s). The test is added through mir_add_test() so that it is picked up
# both by a plain `ctest` invocation and by the `ptest` discovery used in CI.
#
# Crates that link against the in-tree miral (e.g. mir-sys) need to find both
# the generated pkg-config files and the freshly built shared libraries. We
# therefore point pkg-config at the in-tree .pc files and add the build output
# directory to the compile-time and run-time library search paths. When miral
# is installed system-wide these simply take precedence and are otherwise
# harmless.
function(mir_add_cargo_test)
  set(one_value_args NAME)
  set(multi_value_args CRATES)
  cmake_parse_arguments(ARG "" "${one_value_args}" "${multi_value_args}" ${ARGN})

  if("${ARG_NAME}" STREQUAL "")
    message(FATAL_ERROR "mir_add_cargo_test called without NAME <value>")
  endif()

  if("${ARG_CRATES}" STREQUAL "")
    message(FATAL_ERROR "mir_add_cargo_test called without CRATES <value>...")
  endif()

  if(NOT CARGO_EXECUTABLE)
    message(FATAL_ERROR "mir_add_cargo_test requires cargo, but it was not found")
  endif()

  set(package_args)
  foreach(crate IN LISTS ARG_CRATES)
    list(APPEND package_args -p ${crate})
  endforeach()

  if("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
    set(cargo_release_flag --release)
  else()
    set(cargo_release_flag)
  endif()

  # Locations of the in-tree, generated pkg-config files needed to resolve
  # miral and its `Requires:` (mircore).
  set(pkg_config_path
    "${CMAKE_BINARY_DIR}/src/miral:${CMAKE_BINARY_DIR}/src/core:$ENV{PKG_CONFIG_PATH}")

  # Where the freshly built shared libraries (libmiral, libmircore, ...) live.
  set(lib_dir "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")

  mir_add_test(NAME ${ARG_NAME}
    COMMAND ${CMAKE_COMMAND} -E env
      "PKG_CONFIG_PATH=${pkg_config_path}"
      "RUSTFLAGS=-L native=${lib_dir}"
      "LD_LIBRARY_PATH=${lib_dir}:$ENV{LD_LIBRARY_PATH}"
      ${CARGO_EXECUTABLE} test --target-dir ${CMAKE_BINARY_DIR}/target ${cargo_release_flag} ${package_args}
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})
endfunction()
