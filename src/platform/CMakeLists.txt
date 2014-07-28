include_directories(${PROJECT_SOURCE_DIR}/include/platform)
include_directories(${UDEV_INCLUDE_DIRS})

add_library(
  mirplatform SHARED
  shared_library_loader.cpp
)

target_link_libraries(
  mirplatform
  mirplatformgraphicscommon
  mircommon
)

install(TARGETS mirplatform LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

add_subdirectory(graphics/)
add_subdirectory(options)
