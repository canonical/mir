
include_directories(
  ${PROJECT_SOURCE_DIR}/src/include/server
  ${PROJECT_SOURCE_DIR}/src/include/platform
  ${PROJECT_SOURCE_DIR}/src/include/common
  ${PROJECT_SOURCE_DIR}/src/include/gl
  ${PROJECT_SOURCE_DIR}/src/include/client
  ${PROJECT_SOURCE_DIR}/src/renderers
  ${PROJECT_SOURCE_DIR}/include/client
  ${PROJECT_SOURCE_DIR}/include/server
  ${PROJECT_SOURCE_DIR}/include/platform
  ${PROJECT_SOURCE_DIR}/include/renderer
  ${PROJECT_SOURCE_DIR}/include/renderers/gl
  ${PROJECT_SOURCE_DIR}/examples/
)

add_library(playgroundserverconfig STATIC
    server_configuration.cpp
)

add_subdirectory(demo-shell/)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99")
mir_add_wrapped_executable(mir_demo_client_prerendered_frames
    mir_demo_client_prerendered_frames.c
)

target_link_libraries(mir_demo_client_prerendered_frames
    mirclient
    m
)

mir_add_wrapped_executable(mir_demo_client_chain_jumping_buffers
    mir_demo_client_chain_jumping_buffers.c
)

target_link_libraries(mir_demo_client_chain_jumping_buffers
    mirclient
)

mir_add_wrapped_executable(mir_demo_client_render_surface
  render_surface.cpp
)

target_link_libraries(mir_demo_client_render_surface
    mirclient
    eglapp
)

mir_add_wrapped_executable(mir_demo_client_egldiamond_render_surface
    egldiamond_render_surface.c
    mir_egl_platform_shim.c
    diamond.c
)

target_link_libraries(mir_demo_client_egldiamond_render_surface
    mirclient
    ${EGL_LIBRARIES}
    ${GLESv2_LIBRARIES}
)
