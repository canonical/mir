include_directories(
  ${server_common_include_dirs}
)

add_library(
  mirplatforminputx11objects OBJECT

  input_platform.cpp
  input_device.cpp
)

target_link_libraries(mirplatforminputx11objects
  PUBLIC
    mirplatform
    mircommon
    mircore
)

add_library(
  mirplatforminputx11objects-symbols OBJECT

  input.cpp
)

target_link_libraries(mirplatforminputx11objects-symbols
  PUBLIC
    mirplatform
    mircommon
)
