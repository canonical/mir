set(XDG_SHELL_H "${CMAKE_CURRENT_BINARY_DIR}/xdg-shell.h")
set(XDG_SHELL_C "${CMAKE_CURRENT_BINARY_DIR}/xdg-shell.c")
set(XDG_SHELL_X "${PROJECT_SOURCE_DIR}/wayland-protocols/xdg-shell.xml")

add_custom_command(
      OUTPUT "${XDG_SHELL_H}" "${XDG_SHELL_C}"
      VERBATIM
      COMMAND "sh" "-c" "wayland-scanner client-header ${XDG_SHELL_X} ${XDG_SHELL_H}"
      COMMAND "sh" "-c" "wayland-scanner private-code  ${XDG_SHELL_X} ${XDG_SHELL_C}"
)

set(MIR_SHELL_H "${CMAKE_CURRENT_BINARY_DIR}/mir-shell.h")
set(MIR_SHELL_C "${CMAKE_CURRENT_BINARY_DIR}/mir-shell.c")
set(MIR_SHELL_X "${PROJECT_SOURCE_DIR}/wayland-protocols/mir-shell-unstable-v1.xml")

add_custom_command(
        OUTPUT "${MIR_SHELL_H}" "${MIR_SHELL_C}"
        VERBATIM
        COMMAND "sh" "-c" "wayland-scanner client-header ${MIR_SHELL_X} ${MIR_SHELL_H}"
        COMMAND "sh" "-c" "wayland-scanner private-code  ${MIR_SHELL_X} ${MIR_SHELL_C}"
)

add_library(mir_demo_wayland_extensions STATIC
        ${XDG_SHELL_C} ${XDG_SHELL_H}
        ${MIR_SHELL_C} ${MIR_SHELL_H}
        make_shm_pool.c make_shm_pool.h
)
set_target_properties     (mir_demo_wayland_extensions PROPERTIES COMPILE_FLAGS "${CMAKE_CFLAGS}  -fvisibility=hidden")
target_include_directories(mir_demo_wayland_extensions PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries     (mir_demo_wayland_extensions PUBLIC PkgConfig::WAYLAND_CLIENT)


mir_add_wrapped_executable(mir_demo_client_wayland wayland_client.c)
target_link_libraries     (mir_demo_client_wayland mir_demo_wayland_extensions)
target_include_directories(mir_demo_client_wayland PRIVATE ${CMAKE_CURRENT_BINARY_DIR})


mir_add_wrapped_executable(mir_demo_client_wayland_egl_spinner spinner.cpp)
target_link_libraries     (mir_demo_client_wayland_egl_spinner miral-spinner)


mir_add_wrapped_executable(mir_demo_client_mir_shell mir_shell_demo.cpp wayland_runner.cpp wayland_runner.h)
target_link_libraries     (mir_demo_client_mir_shell mir_demo_wayland_extensions)
target_include_directories(mir_demo_client_mir_shell PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
