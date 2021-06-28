include_directories(
  ${server_common_include_dirs}
  ${PROJECT_SOURCE_DIR}/include/client
)

add_library(
  mirplatforminputx11objects OBJECT

  input_platform.cpp
  input_device.cpp
)

target_link_libraries(mirplatforminputx11objects mircore)

add_library(
  mirplatforminputx11objects-symbols OBJECT

  input.cpp
)
