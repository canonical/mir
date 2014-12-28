set(RENDER_SURFACES_SOURCES
    render_surfaces.cpp
    buffer_render_target.cpp
    image_renderer.cpp
)
add_executable(mir_demo_standalone_render_surfaces ${RENDER_SURFACES_SOURCES})
target_link_libraries(mir_demo_standalone_render_surfaces
    mirserver
    exampleserverconfig
    ${Boost_LIBRARIES}
)

install(TARGETS mir_demo_standalone_render_surfaces RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 -Wall -fno-strict-aliasing -Wextra")

add_library(eglapp STATIC
  eglapp.c
)

add_library(exampleserverconfig STATIC
  server_example_display_configuration_policy.cpp
  server_example_input_event_filter.cpp
  server_example_log_options.cpp
  server_example_input_filter.cpp
  server_example_fullscreen_placement_strategy.cpp
  server_example_host_lifecycle_event.cpp
)

target_link_libraries(eglapp
  mirclient
  ${EGL_LIBRARIES}
  ${GLESv2_LIBRARIES}
)
mir_add_wrapped_executable(mir_demo_client_eglflash
  eglflash.c
)
target_link_libraries(mir_demo_client_eglflash
  eglapp
)
mir_add_wrapped_executable(mir_demo_client_egltriangle
  egltriangle.c
)
target_link_libraries(mir_demo_client_egltriangle
  eglapp
)
mir_add_wrapped_executable(mir_demo_client_eglcounter
  eglcounter.cpp
)
target_link_libraries(mir_demo_client_eglcounter
  eglapp
)
mir_add_wrapped_executable(mir_demo_client_eglplasma
  eglplasma.c
)
target_link_libraries(mir_demo_client_eglplasma
  eglapp
)
mir_add_wrapped_executable(mir_demo_client_cursors
  cursors_demo_client.c
)
target_link_libraries(mir_demo_client_cursors
  eglapp
)

mir_add_wrapped_executable(mir_demo_client_basic
  basic.c
)

target_link_libraries(mir_demo_client_basic
  mirclient

  ${Boost_LIBRARIES}
  ${PROTOBUF_LIBRARIES}
  ${CMAKE_THREAD_LIBS_INIT}
)

mir_add_wrapped_executable(mir_demo_client_release_at_exit release_at_exit.c)
target_link_libraries(mir_demo_client_release_at_exit mirclient)

mir_add_wrapped_executable(mir_demo_client_multiwin multiwin.c)
target_link_libraries(mir_demo_client_multiwin mirclient)

mir_add_wrapped_executable(mir_demo_client_fingerpaint fingerpaint.c)
target_link_libraries(mir_demo_client_fingerpaint mirclient)

mir_add_wrapped_executable(mir_demo_client_progressbar progressbar.c)
target_link_libraries(mir_demo_client_progressbar mirclient)

mir_add_wrapped_executable(mir_demo_client_display_config demo_client_display_config.c)
target_link_libraries(mir_demo_client_display_config eglapp)

mir_add_wrapped_executable(mir_demo_client_flicker
  flicker.c
)

target_link_libraries(mir_demo_client_flicker
  mirclient

  ${Boost_LIBRARIES}
  ${PROTOBUF_LIBRARIES}
  ${CMAKE_THREAD_LIBS_INIT}
)

mir_add_wrapped_executable(mir_demo_client_scroll
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

mir_add_wrapped_executable(mir_demo_client_prompt_session prompt_session.c)
target_link_libraries(mir_demo_client_prompt_session mirclient)

add_library(mirdraw STATIC graphics_utils.cpp)
target_link_libraries(mirdraw ${GLESv2_LIBRARIES})

include_directories(
  ${PROJECT_SOURCE_DIR}/include/server
  ${PROJECT_SOURCE_DIR}/include/client
  ${PROJECT_SOURCE_DIR}/include/platform
  ${GLESv2_INCLUDE_DIRS}
)

mir_add_wrapped_executable(mir_demo_standalone_render_to_fb
    render_to_fb.cpp
)

target_link_libraries(mir_demo_standalone_render_to_fb
    mirserver
    mirdraw
    ${Boost_LIBRARIES}
)

mir_add_wrapped_executable(mir_demo_server
  server_example.cpp
  glog_logger.cpp
  server_example_test_client.cpp
)

target_link_libraries(mir_demo_server
  mirserver
  exampleserverconfig
  ${GLog_LIBRARY}
  ${GFlags_LIBRARY}
  ${Boost_LIBRARIES}
)

mir_add_wrapped_executable(mir_demo_server_minimal server_minimal.cpp)
target_link_libraries(mir_demo_server_minimal mirserver)

mir_add_wrapped_executable(mir_demo_standalone_render_overlays
    render_overlays.cpp
)

target_link_libraries(mir_demo_standalone_render_overlays
    mirserver
    mircommon
)
