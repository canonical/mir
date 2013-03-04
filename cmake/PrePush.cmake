# Copyright © 2012 Canonical Ltd.
#
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
# Authored by: Thomas Voss <thomas.voss@canonical.com>

#######################################################################
# A convenience target that carries out the following steps:
#   - Apply astyle to all source files of interest.
#   - Build & test in a chroot, comparable setup to CI/Autolanding
#     and ppa builders. Will fail if new files have not been added.
#   - Build & test for android.
#
# NOTE: This target is very sensitive to the availability of all
#       all required dependencies. For that, we prefer to fail the
#       target if deps are missing to make the problem very visible.
#
# TODO:
#   - Wire up the style-check target once we have reached a state
#     where trunk actually passes the style check.
#######################################################################
add_custom_target(
  pre-push

  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}  
)

#######################################################################
#      Add target for running astyle with the correct options         #
#######################################################################
find_program(ASTYLE_EXECUTABLE astyle)

if(NOT ASTYLE_EXECUTABLE)
  message(STATUS "Could NOT find astyle, pre-push is going to FAIL")
endif()

set(
  ASTYLE_OPTIONS 
  ${ASTYLE_OPTIONS} 
  --indent-switches
  --pad-header
  --unpad-paren
  --align-pointer=type
)

add_custom_target(
  astyle

  find ${CMAKE_SOURCE_DIR}/examples -name *.h | xargs ${ASTYLE_EXECUTABLE} ${ASTYLE_OPTIONS}
  COMMAND find ${CMAKE_SOURCE_DIR}/include    -name *.h | xargs ${ASTYLE_EXECUTABLE} 
  COMMAND find ${CMAKE_SOURCE_DIR}/src        -name *.h | xargs ${ASTYLE_EXECUTABLE} ${${ASTYLE_EXECUTABLE}_OPTIONS}
  COMMAND find ${CMAKE_SOURCE_DIR}/tests      -name *.h | xargs ${ASTYLE_EXECUTABLE} ${${ASTYLE_EXECUTABLE}_OPTIONS} 
  COMMAND find ${CMAKE_SOURCE_DIR}/tools      -name *.h | xargs ${ASTYLE_EXECUTABLE} ${${ASTYLE_EXECUTABLE}_OPTIONS} 

  COMMAND find ${CMAKE_SOURCE_DIR}/examples   -name *.cpp | xargs ${ASTYLE_EXECUTABLE} ${${ASTYLE_EXECUTABLE}_OPTIONS} 
  COMMAND find ${CMAKE_SOURCE_DIR}/include    -name *.cpp | xargs ${ASTYLE_EXECUTABLE} ${${ASTYLE_EXECUTABLE}_OPTIONS} 
  COMMAND find ${CMAKE_SOURCE_DIR}/src        -name *.cpp | xargs ${ASTYLE_EXECUTABLE} ${${ASTYLE_EXECUTABLE}_OPTIONS} 
  COMMAND find ${CMAKE_SOURCE_DIR}/tests      -name *.cpp | xargs ${ASTYLE_EXECUTABLE} ${${ASTYLE_EXECUTABLE}_OPTIONS} 
  COMMAND find ${CMAKE_SOURCE_DIR}/tools      -name *.cpp | xargs ${ASTYLE_EXECUTABLE} ${${ASTYLE_EXECUTABLE}_OPTIONS} 

  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR} VERBATIM
)

#######################################################################
#      Add target for creating a source tarball with bzr export       #
#######################################################################
add_custom_target(
  pre-push-source-tarball

  COMMAND rm -rf pre-push-build-area
  COMMAND mkdir pre-push-build-area
  COMMAND bzr export --root mir-pre-push pre-push-build-area/mir_${MIR_VERSION_MAJOR}.${MIR_VERSION_MINOR}.${MIR_VERSION_PATCH}.orig.tar.bz2 ${CMAKE_SOURCE_DIR}
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  COMMENT "Preparing source tarball for pre-push build & test"
)

#######################################################################
#      Add target for extracting source tarball for pdebuild          #
#######################################################################
add_custom_target(
  extract-pre-push-tarball
  COMMAND tar -xf mir_${MIR_VERSION_MAJOR}.${MIR_VERSION_MINOR}.${MIR_VERSION_PATCH}.orig.tar.bz2
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/pre-push-build-area VERBATIM
)

#######################################################################
#  Builds & tests the last committed revision of the current branch   #
#######################################################################
find_program(PDEBUILD_EXECUTABLE pdebuild)
if(NOT PDEBUILD_EXECUTABLE)
  message(STATUS "pdebuild NOT found, pre-push is going to FAIL")
endif()

add_custom_target(
  pdebuild
  COMMAND ${PDEBUILD_EXECUTABLE}
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/pre-push-build-area/mir-pre-push 
  COMMENT "Building & testing in chroot'd environment"
  VERBATIM  
)

#######################################################################
#  Builds & tests the last committed revision of the current branch   #
#  for Android                                                        #
#######################################################################
if(NOT DEFINED ENV{MIR_ANDROID_NDK_DIR})
    message(STATUS "Env. variable MIR_ANDROID_NDK_DIR not set, pre-push is going to FAIL")
endif()
if(NOT DEFINED ENV{MIR_ANDROID_SDK_DIR})
    message(STATUS "Env. variable MIR_ANDROID_SDK_DIR not set, pre-push is going to FAIL")
endif()

add_custom_target(
  android-build

  COMMAND ./cross-compile.sh android-build
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  COMMENT "Building & testing for android"
)

#######################################################################
# pdebuild: make tarball -> extract to build area -> pdebuild         #
# android-build: invoke cross-compile script                          #
#######################################################################
add_dependencies(extract-pre-push-tarball pre-push-source-tarball)
add_dependencies(pdebuild extract-pre-push-tarball)

add_dependencies(pre-push pdebuild android-build)