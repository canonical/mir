add_subdirectory(decoration)

set(
  SHELL_SOURCES

  abstract_shell.cpp
  graphics_display_layout.cpp
  graphics_display_layout.h
  default_configuration.cpp
  shell_wrapper.cpp
  system_compositor_window_manager.cpp
  default_persistent_surface_store.cpp
  persistent_surface_store.cpp
  ${CMAKE_SOURCE_DIR}/src/include/server/mir/shell/display_configuration_controller.h
  surface_specification.cpp
  surface_stack_wrapper.cpp ${CMAKE_SOURCE_DIR}/src/include/server/mir/shell/surface_stack_wrapper.h
  basic_idle_handler.cpp ${CMAKE_SOURCE_DIR}/src/include/server/mir/shell/idle_handler.h
  token_authority.cpp ${CMAKE_SOURCE_DIR}/src/include/server/mir/shell/token_authority.h
)

add_library(
  mirshell OBJECT

  ${SHELL_SOURCES}
)

target_link_libraries(mirshell
  PUBLIC
    mirplatform
    mircommon
    mircore
  PRIVATE
    PkgConfig::UUID
)

