# Copyright © 2012 Canonical Ltd.
#
# Permission to use, copy, modify, distribute, and sell this software
# and its documentation for any purpose is hereby granted without
# fee, provided that the above copyright notice appear in all copies
# and that both that copyright notice and this permission notice
# appear in supporting documentation, and that the name of
# Canonical Ltd. not be used in advertising or publicity pertaining to
# distribution of the software without specific, written prior permission.
# Canonical Ltd. makes no representations about the suitability of this
# software for any purpose. It is provided "as is" without express or
# implied warranty.
#
# CANONICAL, LTD. DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
# INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN
# NO EVENT SHALL CANONICAL, LTD. BE LIABLE FOR ANY SPECIAL, INDIRECT OR
# CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
# OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
# NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
# WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
#
# Authored by: Thomas Voss <thomas.voss@canonical.com>,
#              Alan Griffiths <alan@octopull.co.uk>

project(Mir)

cmake_minimum_required(VERSION 2.8)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x -Werror -Wall -pedantic -Wextra")

enable_testing()

include_directories(include/)

add_subdirectory(src/)

# Build Google Test and make its headers known
find_package (GTest)
if(NOT GTEST_FOUND)
  # Check for google test and build it locally
  set(GTEST_ROOT_DIR
    "/usr/src/gtest" # Default value, adjustable by user with e.g., ccmake
    CACHE
    PATH
    "Path to Google test srcs"
    )
  
  find_path(GTEST_INCLUDE_DIR gtest/gtest.h)
  
  if(GTEST_INCLUDE_DIR)
    add_subdirectory(${GTEST_ROOT_DIR} gtest)
  endif(GTEST_INCLUDE_DIR)
  
  set(GTEST_BOTH_LIBRARIES gtest gtest_main)
  set(GTEST_FOUND TRUE)
endif(NOT GTEST_FOUND)

# TODO(tvoss): Move to its own FindGoogleMock module
find_library(GMOCK_LIBRARY gmock)
find_library(GMOCK_MAIN_LIBRARY gmock_main)

if (NOT GMOCK_LIBRARY OR NOT GMOCK_MAIN_LIBRARY OR NOT GTEST_FOUND)
  message ("Google Mock and Google Test not found - cannot build tests!")
endif (NOT GMOCK_LIBRARY OR NOT GMOCK_MAIN_LIBRARY OR NOT GTEST_FOUND)

include_directories (${GTEST_INCLUDE_DIRS})

find_package (Threads REQUIRED)
find_package (Boost REQUIRED)

add_subdirectory(tests/)
add_subdirectory(unit-tests/)
