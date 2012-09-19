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
# Authored by: Kevin DuBois <kevin.dubois@canonical.com>

set(
  TEST_UTILS_SRCS

  graphics_utils.cpp 
)

if (MIR_PLATFORM STREQUAL "android")
  set(
    TEST_UTILS_SRCS

    android_graphics.cpp
    ${TEST_UTILS_SRCS} 
)
endif()

add_library(
  test-utils STATIC 
  ${TEST_UTILS_SRCS})

target_link_libraries(
  test-utils 

  ${GLESv2_LIBRARIES}
)

