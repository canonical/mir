add_subdirectory(common/)

if (MIR_TEST_PLATFORM STREQUAL "mesa-kms")
  add_subdirectory(kms/)
endif()

set(UNIT_TEST_SOURCES ${UNIT_TEST_SOURCES} PARENT_SCOPE)
