include_directories(${PROJECT_SOURCE_DIR}/src/include/client)
include_directories(${PROJECT_SOURCE_DIR}/include/client)

add_library(client_platform_common STATIC
  aging_buffer.cpp
  weak_egl.cpp
)
