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

set(MIR_GENERATED_INCLUDE_DIRECTORIES)
add_subdirectory(protobuf/)
include_directories(${MIR_GENERATED_INCLUDE_DIRECTORIES})

add_subdirectory(compositor/)
add_subdirectory(draw/)
add_subdirectory(graphics/)
add_subdirectory(input/)
add_subdirectory(logging/)
add_subdirectory(surfaces/)
add_subdirectory(options/)
add_subdirectory(client/)
add_subdirectory(frontend/)
add_subdirectory(sessions/)


set(
  SRCS
  display_server.cpp
  server_configuration.cpp
)

add_library(mirserver SHARED
  ${SRCS}
)

add_executable(
  mirds

  main.cpp
)

set_target_properties(mirds PROPERTIES OUTPUT_NAME mir)

target_link_libraries(mirserver
  -Wl,-whole-archive  # libmirserver needs to contain these archives WHOLE...
  miroptions
  mircompositor
  mirfrontend
  mirgraphics
  mirinput
  mirsurfaces
  mirplatformgraphics
  mirprotobuf
  -Wl,-no-whole-archive

  3rd_party

  ${Boost_LIBRARIES}
)

set_target_properties(mirserver PROPERTIES
  VERSION ${MIR_VERSION_MAJOR}.${MIR_VERSION_MINOR}.${MIR_VERSION_PATCH}
  SOVERSION ${MIR_VERSION_MAJOR}
  #
  # Tell CMake that targets using mirserver should NOT re-import the static
  # libraries that mirserver depends on (contains).
  LINK_INTERFACE_LIBRARIES ""
)

target_link_libraries(mirds
  mirserver
  ${Boost_LIBRARIES}
)

install(
  TARGETS mirds
  RUNTIME DESTINATION bin)

set(
  MIR_GENERATED_INCLUDE_DIRECTORIES
  ${MIR_GENERATED_INCLUDE_DIRECTORIES}
  PARENT_SCOPE)
