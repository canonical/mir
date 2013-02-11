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

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 -Wall -fno-strict-aliasing -Wextra")

add_executable(
  mir_demo_client

  demo_client.c
)

target_link_libraries(
  mir_demo_client

  mirclient
  mirprotobuf

  ${Boost_LIBRARIES}
  ${PROTOBUF_LIBRARIES}
  ${CMAKE_THREAD_LIBS_INIT}
)

add_executable(
  mir_demo_client_unaccelerated

  demo_client_unaccelerated.c
)

target_link_libraries(
  mir_demo_client_unaccelerated

  mirclient
  mirprotobuf

  ${Boost_LIBRARIES}
  ${PROTOBUF_LIBRARIES}
  ${CMAKE_THREAD_LIBS_INIT}
)

add_executable(
  mir_demo_client_accelerated

  demo_client_accelerated.cpp
)

target_link_libraries(
  mir_demo_client_accelerated

  mirdraw
  mirclient
  mirprotobuf

  ${Boost_LIBRARIES}
  ${PROTOBUF_LIBRARIES}
  ${CMAKE_THREAD_LIBS_INIT}
  ${EGL_LIBRARIES}
  ${GLESv2_LIBRARIES}

)

include_directories(${GLESv2_INCLUDE_DIRS})

set(RENDER_TO_FB_SOURCES render_to_fb.cpp)
add_executable(render_to_fb ${RENDER_TO_FB_SOURCES})

target_link_libraries(
    render_to_fb

    mirserver
    mirlogging
    mirdraw
    ${Boost_LIBRARIES}
)

set(RENDER_SURFACES_SOURCES render_surfaces.cpp buffer_render_target.cpp image_renderer.cpp)
add_executable(render_surfaces ${RENDER_SURFACES_SOURCES})
target_link_libraries(
    render_surfaces

    mirserver
    mirsessions
    ${Boost_LIBRARIES}
)

if (MIR_TRANSPORT STREQUAL "binder")
target_link_libraries(mir_demo_client binder)
target_link_libraries(mir_demo_client_accelerated binder)
target_link_libraries(mir_demo_client_unaccelerated binder)
endif()

install(
  TARGETS mir_demo_client
  RUNTIME DESTINATION bin)
install(
  TARGETS mir_demo_client_unaccelerated
  RUNTIME DESTINATION bin)
install(
  TARGETS mir_demo_client_accelerated
  RUNTIME DESTINATION bin)
