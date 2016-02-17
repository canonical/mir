include_directories(
  ${PROJECT_SOURCE_DIR}/include/platforms/mesa
  ${PROJECT_SOURCE_DIR}/src/platforms/mesa/server/common
)

add_subdirectory(common/)

if (MIR_BUILD_PLATFORM_MESA_KMS)
  add_subdirectory(kms/)
endif()

if (MIR_BUILD_PLATFORM_MESA_X11)
  add_subdirectory(x11/)
endif()

set(UNIT_TEST_SOURCES ${UNIT_TEST_SOURCES} PARENT_SCOPE)
set(UMOCK_UNIT_TEST_SOURCES ${UMOCK_UNIT_TEST_SOURCES} PARENT_SCOPE)
