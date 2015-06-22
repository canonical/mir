include_directories(
  ${PROJECT_SOURCE_DIR}/src/platforms/mesa/include
  ${PROJECT_SOURCE_DIR}/src/platforms/mesa/server/common
)

add_subdirectory(common/)

if (MIR_TEST_PLATFORM STREQUAL "mesa-kms")
  add_subdirectory(kms/)
endif()

if (MIR_TEST_PLATFORM STREQUAL "mesa-x11")
  add_subdirectory(x11/)
endif()

set(UNIT_TEST_SOURCES ${UNIT_TEST_SOURCES} PARENT_SCOPE)
