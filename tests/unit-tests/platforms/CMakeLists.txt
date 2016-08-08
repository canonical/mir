if(MIR_BUILD_PLATFORM_ANDROID)
  add_subdirectory(android/)
endif()

message(${MIR_PLATFORM})
if (MIR_BUILD_PLATFORM_MESA_KMS OR MIR_BUILD_PLATFORM_MESA_X11)
  add_subdirectory(mesa/)
  list (APPEND UNIT_TEST_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/test_shm_buffer.cpp
  )
endif()

add_subdirectory(nested/)
