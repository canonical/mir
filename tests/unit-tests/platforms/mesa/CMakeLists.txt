include_directories(
  ${PROJECT_SOURCE_DIR}/include/platforms/mesa
  ${PROJECT_SOURCE_DIR}/src/platforms/mesa/server
)

add_subdirectory(kms-utils/)

if (MIR_BUILD_PLATFORM_MESA_KMS)
  add_subdirectory(kms/)
endif()

if (MIR_BUILD_PLATFORM_MESA_X11)
  add_subdirectory(x11/)
endif()

add_subdirectory(client/)

set(UNIT_TEST_SOURCES ${UNIT_TEST_SOURCES} PARENT_SCOPE)
