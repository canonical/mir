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

include_directories(
    ${LIBHARDWARE_INCLUDE_DIRS}
    ${ANDROID_UI_INCLUDE_DIRS}
    ${EGL_INCLUDE_DIRS}
    ${GLESv2_INCLUDE_DIRS}
)

add_library(
  mirplatformgraphics STATIC

  android_platform.cpp
  android_buffer_allocator.cpp
  android_buffer.cpp
  android_buffer_handle.cpp
  android_display.cpp
  android_framebuffer_window.cpp
  android_alloc_adaptor.cpp
)

target_link_libraries(
  mirplatformgraphics
  mircompositor

  ${LIBHARDWARE_LIBRARIES}
  ${ANDROID_UI_LIBRARIES}
  ${EGL_LDFLAGS} ${EGL_LIBRARIES}
  ${GLESv2_LDFLAGS} ${GLESv2_LIBRARIES}
)
