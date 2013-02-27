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
# Authored by: Alan Griffiths <alan@octopull.co.uk>

list(
  APPEND MIR_3RD_PARTY_INCLUDE_DIRECTORIES
  ${CMAKE_CURRENT_SOURCE_DIR}/libancillary
  ${CMAKE_CURRENT_SOURCE_DIR}/glm
  ${CMAKE_CURRENT_SOURCE_DIR}/android-deps
  )

list(
  APPEND MIR_3RD_PARTY_SOURCES 
  ${CMAKE_CURRENT_SOURCE_DIR}/libancillary/fd_send.c 
  ${CMAKE_CURRENT_SOURCE_DIR}/libancillary/fd_recv.c)

add_library(
  3rd_party 

  ${MIR_3RD_PARTY_SOURCES}
)

set(MIR_3RD_PARTY_INCLUDE_DIRECTORIES ${MIR_3RD_PARTY_INCLUDE_DIRECTORIES} PARENT_SCOPE)
SET(MIR_3RD_PARTY_SOURCES ${MIR_3RD_PARTY_SOURCES} PARENT_SCOPE)
set(MIR_INPUT_ANDROID_COMPILE_FLAGS ${MIR_INPUT_ANDROID_COMPILE_FLAGS}
  PARENT_SCOPE)

if (NOT MIR_DISABLE_INPUT)
  include_directories(${CMAKE_CURRENT_SOURCE_DIR}/android-deps)
  add_subdirectory(android-input)
endif()

set(MIR_ANDROID_INCLUDE_DIRECTORIES ${MIR_ANDROID_INCLUDE_DIRECTORIES} PARENT_SCOPE)
set(
  MIR_ANDROID_INPUT_COMPILE_FLAGS
  ${MIR_ANDROID_INPUT_COMPILE_FLAGS}
  PARENT_SCOPE)
  
if (NOT MIR_DISABLE_INPUT)
target_link_libraries(
  3rd_party

  android-input)
endif()
