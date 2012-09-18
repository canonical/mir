
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

if (MIR_PLATFORM STREQUAL "android")
    set(RENDER_TO_FB_EXTRA_LINK_LIBRARIES ${ANDROID_UI_LIBRARIES})
endif()

set(RENDER_TO_FB_SOURCES render_to_fb.cpp)
add_executable(render_to_fb ${RENDER_TO_FB_SOURCES})
target_link_libraries(
    render_to_fb

    mirplatformgraphics
    ${RENDER_TO_FB_EXTRA_LINK_LIBRARIES}
    ${EGL_LIBRARIES}
    ${GLESv2_LIBRARIES}
)
