include_directories(
  ${server_common_include_dirs}
)

add_library(server_platform_common OBJECT
  anonymous_shm_file.cpp
  shm_buffer.cpp
  shm_file.h
)
