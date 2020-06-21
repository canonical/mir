include_directories(
  ${server_common_include_dirs}
  ${PROJECT_SOURCE_DIR}/include/client
)

add_library(
  mirplatforminputx11objects OBJECT

  input_platform.cpp
  input_device.cpp
)

add_library(
  mirplatforminputx11objects-symbols OBJECT

  input.cpp
)
