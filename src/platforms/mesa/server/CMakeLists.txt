if (MIR_BUILD_PLATFORM_MESA)
  add_subdirectory(common/)
endif()

if (MIR_BUILD_PLATFORM_MESA)
  add_subdirectory(KMS/)
endif()
