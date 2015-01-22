include_directories(${PROJECT_SOURCE_DIR}/src/include/client)

add_library(client_platform_common OBJECT
  aging_buffer.cpp
)
