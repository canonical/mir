set(OUTPUT_PATH_HEADER "${CMAKE_CURRENT_BINARY_DIR}/xdg-shell.h")
set(OUTPUT_PATH_SRC "${CMAKE_CURRENT_BINARY_DIR}/xdg-shell.c")
set(PROTOCOL_PATH "/usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml")

add_custom_command(
      OUTPUT "${OUTPUT_PATH_HEADER}" "${OUTPUT_PATH_SRC}"
      VERBATIM
      COMMAND "sh" "-c" "wayland-scanner client-header ${PROTOCOL_PATH} ${OUTPUT_PATH_HEADER}"
      COMMAND "sh" "-c" "wayland-scanner private-code  ${PROTOCOL_PATH} ${OUTPUT_PATH_SRC}"
)

mir_add_wrapped_executable(mir_demo_client_wayland wayland_client.c ${OUTPUT_PATH_HEADER} ${OUTPUT_PATH_SRC})
target_link_libraries     (mir_demo_client_wayland PkgConfig::WAYLAND_CLIENT)
target_include_directories(mir_demo_client_wayland PRIVATE ${CMAKE_CURRENT_BINARY_DIR})


mir_add_wrapped_executable(mir_demo_client_wayland_egl_spinner spinner.cpp)
target_link_libraries     (mir_demo_client_wayland_egl_spinner miral-spinner)
