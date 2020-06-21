include_directories(
  ${PROJECT_SOURCE_DIR}/include/platforms/mesa
  ${PROJECT_SOURCE_DIR}/src/platforms/mesa/server
)

add_subdirectory(kms-utils/)

if (MIR_BUILD_PLATFORM_MESA_KMS AND
    (GBM_VERSION VERSION_GREATER 11.0.0))
  add_subdirectory(kms/)
endif()

set(UNIT_TEST_SOURCES ${UNIT_TEST_SOURCES} PARENT_SCOPE)
