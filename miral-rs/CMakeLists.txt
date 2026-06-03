# Copyright © Canonical Ltd.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 or 3 as
# published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

include(RustLibrary)

set(rust_target_dir "${CMAKE_BINARY_DIR}/target")
if("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
  set(rust_binary_dir "${rust_target_dir}/$ENV{DEB_HOST_RUST_TYPE}/release")
  set(cargo_release_flag "--release")
else()
  set(rust_binary_dir "${rust_target_dir}/$ENV{DEB_HOST_RUST_TYPE}/debug")
  set(cargo_release_flag "")
endif()

set(miral_rs_lib "${rust_binary_dir}/libmiral.rlib")

add_custom_command(
  OUTPUT ${miral_rs_lib}
  COMMAND ${CARGO_EXECUTABLE} build ${cargo_release_flag} --target-dir ${rust_target_dir} -p miral
  DEPENDS Cargo.toml src/lib.rs
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  COMMENT "Building Rust crate miral (miral-rs)"
)

add_custom_target(miral-rs ALL
  DEPENDS ${miral_rs_lib} miral_sys
)
