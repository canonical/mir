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
# Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>

include_directories(
    ${DRM_INCLUDE_DIRS}
    ${GBM_INCLUDE_DIRS}
    ${EGL_INCLUDE_DIRS}
    ${GLESv2_INCLUDE_DIRS}
)

# gbm.h and drm.h have trailing commas at the end of enum definitions
# This is valid C99, but g++ 4.4 flags it as an error with -pedantic
string(REPLACE "-pedantic" "" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})


add_library(
  mirplatformgraphics STATIC

  gbm_platform.cpp
  gbm_buffer_allocator.cpp
  gbm_buffer.cpp
  gbm_display_helpers.cpp
  gbm_display.cpp
  gbm_display_reporter.cpp
  kms_display_configuration.cpp
  drm_mode_resources.cpp
  gbm_display_buffer.cpp
  kms_output.cpp
  kms_output_container.cpp
  kms_page_flipper.cpp
)

target_link_libraries(
  mirplatformgraphics
  mircompositor
  mirlogging

  ${DRM_LDFLAGS} ${DRM_LIBRARIES}
  ${GBM_LDFLAGS} ${GBM_LIBRARIES}
  ${EGL_LDFLAGS} ${EGL_LIBRARIES}
  ${GLESv2_LDFLAGS} ${GLESv2_LIBRARIES}
)
