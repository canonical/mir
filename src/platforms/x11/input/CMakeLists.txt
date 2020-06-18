include_directories(
  ${server_common_include_dirs}
  ${PROJECT_SOURCE_DIR}/include/client
)

add_library(
  mirplatforminputmesax11objects OBJECT

  input_platform.cpp
  input_device.cpp
)

add_library(
  mirplatforminputmesax11objects-symbols OBJECT

  input.cpp
)
