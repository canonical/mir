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
  ${CMAKE_SOURCE_DIR}/src/include/server/mir/shell/accessibility_manager.h
  basic_accessibility_manager.cpp basic_accessibility_manager.h
  mouse_keys_transformer.cpp mouse_keys_transformer.h
  basic_simulated_secondary_click_transformer.cpp basic_simulated_secondary_click_transformer.h
  basic_hover_click_transformer.cpp basic_hover_click_transformer.h
  basic_slow_keys_transformer.cpp basic_slow_keys_transformer.h
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

