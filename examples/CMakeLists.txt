set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 -Wall -fno-strict-aliasing -Wextra")

add_library(eglapp STATIC
  eglapp.c
)
target_link_libraries(eglapp
  mirclient
  ${EGL_LIBRARIES}
  ${GLESv2_LIBRARIES}
)
add_executable(mir_eglflash
  eglflash.c
)
target_link_libraries(mir_eglflash
  eglapp
)
add_executable(mir_egltriangle
  egltriangle.c
)
target_link_libraries(mir_egltriangle
  eglapp
)
add_executable(mir_eglplasma
  eglplasma.c
)
target_link_libraries(mir_eglplasma
  eglapp
)

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

include_directories(
  ${PROJECT_SOURCE_DIR}/include/server
  ${PROJECT_SOURCE_DIR}/include/client
  ${GLESv2_INCLUDE_DIRS}
)

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
    mirshell
    ${Boost_LIBRARIES}
)

if (MIR_TRANSPORT STREQUAL "binder")
target_link_libraries(mir_demo_client binder)
target_link_libraries(mir_demo_client_accelerated binder)
target_link_libraries(mir_demo_client_unaccelerated binder)
endif()

set (DEMO_CLIENTS
  mir_demo_client
  mir_demo_client_unaccelerated
  mir_demo_client_accelerated
  mir_eglflash
  mir_egltriangle
  mir_eglplasma
)

install(TARGETS ${DEMO_CLIENTS} RUNTIME DESTINATION bin)

