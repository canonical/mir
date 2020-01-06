add_library(mirdraw STATIC graphics_utils.cpp graphics.h)
target_include_directories(mirdraw PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

mir_add_wrapped_executable(mir_demo_client_wayland wayland_client.c)
target_link_libraries     (mir_demo_client_wayland ${WAYLAND_CLIENT_LDFLAGS} ${WAYLAND_CLIENT_LIBRARIES})
