
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
add_executable(mir_demo_client_eglplasma
  eglplasma.c
)
target_link_libraries(mir_demo_client_eglplasma
  eglapp
)

add_executable(mir_demo_client_basic
  basic.c
)

target_link_libraries(mir_demo_client_basic
  mirclient
  mirprotobuf

  ${Boost_LIBRARIES}
  ${PROTOBUF_LIBRARIES}
  ${CMAKE_THREAD_LIBS_INIT}
)

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
  mirprotobuf

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
  mirprotobuf

  ${Boost_LIBRARIES}
  ${PROTOBUF_LIBRARIES}
  ${CMAKE_THREAD_LIBS_INIT}
  ${EGL_LIBRARIES}
  ${GLESv2_LIBRARIES}

)

add_library(mirdraw STATIC graphics_utils.cpp)
target_link_libraries(mirdraw ${GLESv2_LIBRARIES})

include_directories(
  ${PROJECT_SOURCE_DIR}/include/server
  ${PROJECT_SOURCE_DIR}/include/client
  ${PROJECT_SOURCE_DIR}/include/platform
  ${GLESv2_INCLUDE_DIRS}
)

set(RENDER_TO_FB_SOURCES render_to_fb.cpp)
add_executable(mir_demo_standalone_render_to_fb 
  ${RENDER_TO_FB_SOURCES}
)

target_link_libraries(mir_demo_standalone_render_to_fb
    mirserver
    mirlogging
    mirdraw
    ${Boost_LIBRARIES}
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
    mirshell
    mirsharedgeometry
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
  mir_demo_standalone_input_filter
  mir_demo_standalone_render_to_fb 
  mir_demo_standalone_render_surfaces
)

add_subdirectory(demo-inprocess-surface-client)
add_subdirectory(demo-shell)

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
