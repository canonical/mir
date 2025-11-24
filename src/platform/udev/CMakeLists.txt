add_library(mirudev OBJECT
  udev_wrapper.cpp
)

target_include_directories(mirudev

  PUBLIC
    ${PROJECT_SOURCE_DIR}/include/platform
)

target_link_libraries(mirudev
  PUBLIC
    PkgConfig::UDEV
)