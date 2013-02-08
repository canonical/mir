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

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/mirserver.pc.in
  ${CMAKE_CURRENT_BINARY_DIR}/mirserver.pc
)

set(
  SRCS
  display_server.cpp
  server_configuration.cpp
)

set(MIRSERVER_LINKAGE SHARED)

add_library(mirserver ${MIRSERVER_LINKAGE}
  ${SRCS}
)

add_executable(
  mirds

  main.cpp
)

set_target_properties(mirds PROPERTIES OUTPUT_NAME mir)

list(APPEND MIRSERVER_ARCHIVES
  miroptions
  mircompositor
  mirfrontend
  mirgraphics
  mirinput
  mirsurfaces
)

list(APPEND MIRSERVER_LINKS
  mirplatformgraphics
  mirprotobuf
  3rd_party
  ${Boost_LIBRARIES}
)

if(${MIRSERVER_LINKAGE} STREQUAL SHARED)
  target_link_libraries(mirserver
    LINK_PRIVATE
      -Wl,-whole-archive
      ${MIRSERVER_ARCHIVES}
      -Wl,-no-whole-archive
    LINK_PUBLIC
      ${MIRSERVER_LINKS}
  )
  install(TARGETS mirserver
    LIBRARY DESTINATION lib
  )
else()
  target_link_libraries(mirserver
    LINK_PUBLIC
      ${MIRSERVER_ARCHIVES}
      ${MIRSERVER_LINKS}
  )
endif()

set_target_properties(mirserver PROPERTIES
  VERSION ${MIR_VERSION_MAJOR}.${MIR_VERSION_MINOR}.${MIR_VERSION_PATCH}
  SOVERSION ${MIR_VERSION_MAJOR}
)

target_link_libraries(mirds
  mirserver
  ${Boost_LIBRARIES}
)

install(TARGETS mirds
  RUNTIME DESTINATION bin
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/mirserver.pc
  DESTINATION lib/pkgconfig
)

set(
  MIR_GENERATED_INCLUDE_DIRECTORIES
  ${MIR_GENERATED_INCLUDE_DIRECTORIES}
  PARENT_SCOPE)
