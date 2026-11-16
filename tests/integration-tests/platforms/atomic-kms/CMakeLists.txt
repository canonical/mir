CMAKE_DEPENDENT_OPTION(
  MIR_RUN_VKMS_TESTS
  "Run privileged VKMS atomic-kms integration tests"
  OFF
  "MIR_BUILD_INTEGRATION_TESTS;MIR_BUILD_PLATFORM_ATOMIC_KMS"
  OFF)

mir_add_wrapped_executable(mir_atomic_kms_vkms_tests NOINSTALL
  test_atomic_kms_vkms.cpp
  vkms_device.cpp
  vkms_device.h
)

add_dependencies(mir_atomic_kms_vkms_tests
  GMock
  mirplatformgraphicsatomickms
)

target_include_directories(mir_atomic_kms_vkms_tests PRIVATE
  ${PROJECT_SOURCE_DIR}
  ${PROJECT_SOURCE_DIR}/src/include/server
)

target_link_libraries(mir_atomic_kms_vkms_tests
  mirplatform
  mirserver-static
  mir-test-static
  mir-test-framework-static
  mirnullreport
  Boost::program_options
  PkgConfig::DRM
)

if (MIR_RUN_VKMS_TESTS)
  mir_add_test(NAME atomic-kms-vkms
    COMMAND
      ${PROJECT_SOURCE_DIR}/tools/run_atomic_kms_vkms_tests.sh
      ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/mir_atomic_kms_vkms_tests
  )
endif()
