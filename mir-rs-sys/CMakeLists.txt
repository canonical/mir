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

add_rust_cxx_library(mir_sys
  CRATE mir-sys
  DEPENDS Cargo.toml build.rs src/lib.rs src/bridge.cpp src/bridge.h
  LIBRARIES miral mircommon mircore
  INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Object library for compile_commands.json export.
# The actual compilation of bridge.cpp is handled by Cargo via build.rs,
# but this target ensures clangd and other tools can find the source.
add_library(mir-rs-sys-compile-commands OBJECT
  src/bridge.cpp
  src/bridge.h
)

add_dependencies(mir-rs-sys-compile-commands mir_sys-rust-build)

target_include_directories(mir-rs-sys-compile-commands
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    "$<TARGET_PROPERTY:mir_sys,INTERFACE_INCLUDE_DIRECTORIES>"
    "$<TARGET_PROPERTY:miral,INTERFACE_INCLUDE_DIRECTORIES>"
    "$<TARGET_PROPERTY:mircommon,INTERFACE_INCLUDE_DIRECTORIES>"
    "$<TARGET_PROPERTY:mircore,INTERFACE_INCLUDE_DIRECTORIES>"
)

target_link_libraries(mir-rs-sys-compile-commands
  PRIVATE
    miral
    mircommon
    mircore
)

set_target_properties(mir-rs-sys-compile-commands PROPERTIES
  EXCLUDE_FROM_ALL TRUE
)
