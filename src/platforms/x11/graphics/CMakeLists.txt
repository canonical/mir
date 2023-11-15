include_directories(
  ${server_common_include_dirs}
)

add_library(
  mirplatformgraphicsx11objects OBJECT

  platform.cpp
  display.cpp
  display_configuration.cpp
  display_sink.cpp
  egl_helper.cpp
)

add_library(
  mirplatformgraphicsx11objects-symbols OBJECT

  graphics.cpp
)

target_link_libraries(mirplatformgraphicsx11objects-symbols
  PUBLIC
    mirplatform
    mircommon
    mircore
)

target_link_libraries(
  mirplatformgraphicsx11objects

  PRIVATE
    mirplatform
)
