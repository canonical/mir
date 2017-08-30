include_directories(
  ${PROJECT_SOURCE_DIR}/include/client
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
