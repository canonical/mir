include_directories(
  ${PROJECT_SOURCE_DIR}/include/platforms/gbm
        ${PROJECT_SOURCE_DIR}/src/platforms/gbm-kms/server
)

add_subdirectory(kms-utils/)

if (MIR_BUILD_PLATFORM_GBM_KMS)
  add_subdirectory(kms/)
endif()

set(UNIT_TEST_SOURCES ${UNIT_TEST_SOURCES} PARENT_SCOPE)
