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

if (DEFINED CAPNP_LIBRARIES_LITE)
  set(CAPNPC_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR})

  capnp_generate_cpp(CAPNPROTO_SRC CAPNPROTO_HDRS mir_event.capnp)

  add_library(mircapnproto STATIC ${CAPNPROTO_SRC})
  target_link_libraries(mircapnproto ${CAPNP_LIBRARIES_LITE})
# TODO Remove this entire else statement once vivid has libproto 0.5.3 or greater
else ()
  find_library(CAPNPLIB capnp)
  if(NOT CAPNPLIB)
    message(FATAL_ERROR "libcapnp-dev not found.")
  endif()

  find_library(KJLIB kj)
  if(NOT KJLIB)
    message(FATAL_ERROR "capnproto not found.")
  endif()

  find_program(CAPNPC_BIN capnpc)
  if(NOT CAPNPC_BIN)
    message(FATAL_ERROR "Capnp compiler not found.")
  endif()
  set(CAPNPC_FLAGS "" CACHE STRING "Extra flags for capnpc.")

  set(CAPNPROTO_FILES ${CAPNPROTO_FILES}
      "${CMAKE_CURRENT_SOURCE_DIR}/mir_event.capnp")

  # Custom rules to compile .capnp files
  # Taken from lp:unity-scopes-api
  foreach(file ${CAPNPROTO_FILES})

    string(REPLACE "${CMAKE_SOURCE_DIR}" "${CMAKE_BINARY_DIR}" build_dir ${file})
    get_filename_component(build_dir ${build_dir} PATH)

    get_filename_component(proto_file ${file} NAME)
    string(REPLACE ".capnp" ".capnp.c++" src_file ${proto_file})
    string(REPLACE ".capnp" ".capnp.h" hdr_file ${proto_file})

    set(src_file ${build_dir}/${src_file})
    set(hdr_file ${build_dir}/${hdr_file})

    set(CAPNPROTO_HDRS ${CAPNPROTO_HDRS} ${hdr_file})
    set(CAPNPROTO_SRC ${CAPNPROTO_SRC} ${src_file})
  endforeach()

  add_custom_command(OUTPUT  ${CAPNPROTO_SRC} ${CAPNPROTO_HDRS}
                     DEPENDS ${CAPNPROTO_FILES}
                     COMMAND ${CAPNPC_BIN} ${CAPNPC_FLAGS} -oc++:${CMAKE_BINARY_DIR} --src-prefix=${CMAKE_SOURCE_DIR} ${CAPNPROTO_FILES})

  add_library(mircapnproto STATIC ${CAPNPROTO_SRC})
  target_link_libraries(mircapnproto ${CAPNPLIB} ${KJLIB})
endif()

list(APPEND MIR_GENERATED_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_BINARY_DIR})

set(
  MIR_GENERATED_INCLUDE_DIRECTORIES ${MIR_GENERATED_INCLUDE_DIRECTORIES}
  PARENT_SCOPE
)
