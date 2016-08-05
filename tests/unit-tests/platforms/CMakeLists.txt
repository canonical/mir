if(MIR_TEST_PLATFORM STREQUAL "android")
  add_subdirectory("android")
endif()

if (MIR_TEST_PLATFORM STREQUAL "mesa-kms" OR MIR_TEST_PLATFORM STREQUAL "mesa-x11")
  add_subdirectory(kms-utils/)
  add_subdirectory(mesa/)
  list (APPEND UNIT_TEST_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/test_shm_buffer.cpp
  )
endif()
