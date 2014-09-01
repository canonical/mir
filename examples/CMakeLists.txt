
#its a bit troubling that these need to be included for demo_input_filter
include_directories(${MIR_3RD_PARTY_INCLUDE_DIRECTORIES})
include_directories(${MIR_ANDROID_INCLUDE_DIRECTORIES})
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 -Wall -fno-strict-aliasing -Wextra")

add_library(eglapp STATIC
  eglapp.c
)
target_link_libraries(eglapp
  mirclient
  ${EGL_LIBRARIES}
  ${GLESv2_LIBRARIES}
)
add_executable(mir_demo_client_eglflash
  eglflash.c
)
target_link_libraries(mir_demo_client_eglflash
  eglapp
)
add_executable(mir_demo_client_egltriangle
  egltriangle.c
)
target_link_libraries(mir_demo_client_egltriangle
  eglapp
)
add_executable(mir_demo_client_eglcounter
  eglcounter.cpp
)
target_link_libraries(mir_demo_client_eglcounter
  eglapp
)
add_executable(mir_demo_client_eglplasma
  eglplasma.c
)
target_link_libraries(mir_demo_client_eglplasma
  eglapp
)
add_executable(mir_demo_client_cursors
  cursors_demo_client.c
)
target_link_libraries(mir_demo_client_cursors
  eglapp
)

add_executable(mir_demo_client_basic
  basic.c
)

target_link_libraries(mir_demo_client_basic
  mirclient

  ${Boost_LIBRARIES}
  ${PROTOBUF_LIBRARIES}
  ${CMAKE_THREAD_LIBS_INIT}
)

add_executable(mir_test_client_release_at_exit release_at_exit.c)
target_link_libraries(mir_test_client_release_at_exit mirclient)

add_executable(mir_demo_client_multiwin multiwin.c)
target_link_libraries(mir_demo_client_multiwin mirclient)

add_executable(mir_demo_client_fingerpaint fingerpaint.c)
target_link_libraries(mir_demo_client_fingerpaint mirclient)

add_executable(mir_demo_client_progressbar progressbar.c)
target_link_libraries(mir_demo_client_progressbar mirclient)

add_executable(mir_demo_client_display_config demo_client_display_config.c)
target_link_libraries(mir_demo_client_display_config eglapp)

add_executable(mir_demo_client_flicker
  flicker.c
)

target_link_libraries(mir_demo_client_flicker
  mirclient

  ${Boost_LIBRARIES}
  ${PROTOBUF_LIBRARIES}
  ${CMAKE_THREAD_LIBS_INIT}
)

add_executable(mir_demo_client_scroll
  scroll.cpp
)

target_link_libraries(mir_demo_client_scroll
  mirdraw
  mirclient

  ${Boost_LIBRARIES}
  ${PROTOBUF_LIBRARIES}
  ${CMAKE_THREAD_LIBS_INIT}
  ${EGL_LIBRARIES}
  ${GLESv2_LIBRARIES}

)

add_executable(mir_demo_client_prompt_session prompt_session.c)
target_link_libraries(mir_demo_client_prompt_session mirclient)

add_library(mirdraw STATIC graphics_utils.cpp)
target_link_libraries(mirdraw ${GLESv2_LIBRARIES})

include_directories(
  ${PROJECT_SOURCE_DIR}/include/server
  ${PROJECT_SOURCE_DIR}/include.private/server     # XXX Examples shouldn't
  ${PROJECT_SOURCE_DIR}/include/client             #  need private symbols but
  ${PROJECT_SOURCE_DIR}/include/platform           #  this is better than
  ${PROJECT_SOURCE_DIR}/include.private/platform   #  making them public.
  ${GLESv2_INCLUDE_DIRS}
)

add_executable(mir_demo_standalone_render_to_fb
    render_to_fb.cpp
)

target_link_libraries(mir_demo_standalone_render_to_fb
    mirserver
    mirdraw
    ${Boost_LIBRARIES}
)

add_executable(mir_demo_standalone_render_overlays
    render_overlays.cpp
)

target_link_libraries(mir_demo_standalone_render_overlays
    mirserver
    mirtestdraw
    mircommon
)

set(RENDER_SURFACES_SOURCES
    render_surfaces.cpp
    buffer_render_target.cpp
    image_renderer.cpp
    server_configuration.cpp
)
add_executable(mir_demo_standalone_render_surfaces ${RENDER_SURFACES_SOURCES})
target_link_libraries(mir_demo_standalone_render_surfaces
    mirserver
    ${Boost_LIBRARIES}
)

add_executable(mir_demo_standalone_input_filter
  demo_input_filter.cpp
  server_configuration.cpp
)

target_link_libraries(mir_demo_standalone_input_filter
  mirserver
)

set (INSTALL_DEMOS
  mir_demo_client_basic
  mir_demo_client_flicker
  mir_demo_client_scroll
  mir_demo_client_eglflash
  mir_demo_client_egltriangle
  mir_demo_client_eglplasma
  mir_demo_client_fingerpaint
  mir_demo_client_multiwin
  mir_demo_client_display_config
  mir_demo_client_progressbar
  mir_demo_client_prompt_session
  mir_demo_standalone_input_filter
  mir_demo_standalone_render_to_fb
  mir_demo_standalone_render_surfaces
)

add_subdirectory(demo-inprocess-surface-client)
add_subdirectory(demo-shell)
add_subdirectory(testdraw)

install(TARGETS ${INSTALL_DEMOS} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

add_executable(mir_demo_server_basic
  basic_server.cpp
  basic_server_configuration.cpp
  server_configuration.cpp
)

target_link_libraries(mir_demo_server_basic
  mirserver
  ${Boost_LIBRARIES}
)

install(TARGETS mir_demo_server_basic
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

add_executable(mir_demo_server_minimal minimal_server.cpp)
target_link_libraries(mir_demo_server_minimal mirserver)
install(TARGETS mir_demo_server_minimal
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

add_executable(mir_demo_server_translucent
  translucent_server.cpp
  translucent_server_configuration.cpp
  pixel_format_selector.cpp
  basic_server_configuration.cpp
  server_configuration.cpp
)

target_link_libraries(mir_demo_server_translucent
  mirserver
  ${Boost_LIBRARIES}
)

install(TARGETS mir_demo_server_translucent
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
