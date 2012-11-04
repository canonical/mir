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
add_subdirectory(graphics/)
add_subdirectory(input/)
add_subdirectory(logging/)
add_subdirectory(process/)
add_subdirectory(surfaces/)
add_subdirectory(options/)
add_subdirectory(client/)
add_subdirectory(frontend/)


set(
  SRCS
  display_server.cpp
  server_configuration.cpp
)

add_library(
  mir STATIC

  ${SRCS}
)

add_dependencies(mir mirprotobuf)

add_executable(
  mirds
  
  main.cpp
  ${MIR_3RD_PARTY_SOURCES}
)

set_target_properties(mirds PROPERTIES OUTPUT_NAME mir)

target_link_libraries(
  mirds
  
  mir
  miroptions
  mircompositor
  mirfrontend
  mirgraphics
  mirinput
  mirprocess
  mirsurfaces
  mirplatformgraphics
  mirprotobuf
 
  ${PROTOBUF_LIBRARIES}
  ${Boost_LIBRARIES}
  ${GTEST_BOTH_LIBRARIES}
  ${GMOCK_LIBRARY}
  ${GMOCK_MAIN_LIBRARY}
  ${CMAKE_THREAD_LIBS_INIT}
  ${GLESv2_LIBRARIES}
)

install(
  TARGETS mirds
  RUNTIME DESTINATION bin)
  
set(
  MIR_GENERATED_INCLUDE_DIRECTORIES 
  ${MIR_GENERATED_INCLUDE_DIRECTORIES} 
  PARENT_SCOPE)
