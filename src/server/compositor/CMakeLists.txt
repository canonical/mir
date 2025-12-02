set(
  MIR_COMPOSITOR_SRCS

  default_display_buffer_compositor.cpp
  default_display_buffer_compositor_factory.cpp
  multi_threaded_compositor.cpp
  occlusion.cpp
  default_configuration.cpp
  stream.cpp
  ${PROJECT_SOURCE_DIR}/src/include/server/mir/compositor/stream.h
  multi_monitor_arbiter.cpp
  basic_screen_shooter.cpp
  null_screen_shooter.cpp
  basic_screen_shooter_factory.cpp
  null_screen_shooter_factory.cpp
)

ADD_LIBRARY(
  mircompositor OBJECT

  ${MIR_COMPOSITOR_SRCS}
)

target_include_directories(mircompositor
  PUBLIC
    ${PROJECT_SOURCE_DIR}/include/server
  PRIVATE
    ${PROJECT_SOURCE_DIR}/src/include/server
)

target_link_libraries(mircompositor
  PUBLIC
    mirplatform
    mircommon
    mircore
)
