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

add_executable(mir_demo_client
  demo_client.c
)

target_link_libraries(mir_demo_client
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

add_executable(mir_demo_client_unaccelerated
  demo_client_unaccelerated.c
)

target_link_libraries(mir_demo_client_unaccelerated
  mirclient
  mirprotobuf

  ${Boost_LIBRARIES}
  ${PROTOBUF_LIBRARIES}
  ${CMAKE_THREAD_LIBS_INIT}
)

add_executable(mir_demo_client_accelerated
  demo_client_accelerated.cpp
)

target_link_libraries(mir_demo_client_accelerated
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
    ${Boost_LIBRARIES}
)

add_executable(mir_demo_standalone_input_filter
  demo_input_filter.cpp
  server_configuration.cpp
)

target_link_libraries(mir_demo_standalone_input_filter  
  mirserver
)

set (DEMO_CLIENTS
  mir_demo_client
  mir_demo_client_unaccelerated
  mir_demo_client_accelerated
  mir_demo_standalone_input_filter
  mir_demo_client_eglflash
  mir_demo_client_egltriangle
  mir_demo_client_eglplasma
  mir_demo_client_fingerpaint
)

add_subdirectory(demo-inprocess-surface-client)
add_subdirectory(demo-shell)

install(TARGETS ${DEMO_CLIENTS} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

add_executable(mir_demo_server
  demo_server.cpp
  server_configuration.cpp
)

target_link_libraries(mir_demo_server
  mirserver
  ${Boost_LIBRARIES}
)

install(TARGETS mir_demo_server
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

