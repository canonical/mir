if (MIR_BUILD_PLATFORM_MESA_KMS OR MIR_BUILD_PLATFORM_MESA_X11 OR MIR_BUILD_PLATFORM_EGLSTREAM_KMS)
  add_subdirectory(kms-utils/)
endif()

include_directories(
  ${server_common_include_dirs}
  ${GL_INCLUDE_DIRS}
)

add_library(server_platform_common STATIC
  anonymous_shm_file.cpp
  shm_buffer.cpp
  shm_file.h
)

target_link_libraries(
  server_platform_common

  ${KMS_UTILS_STATIC_LIBRARY}
  ${Boost_SYSTEM_LIBRARY}
  ${GL_LDFLAGS} ${GL_LIBRARIES}
)
