if (MIR_BUILD_PLATFORM_MESA)
  add_subdirectory(common/)
endif()

if (MIR_BUILD_PLATFORM_KMS)
  add_subdirectory(KMS/)
endif()

if (MIR_BUILD_PLATFORM_X)
  add_subdirectory(X/)
endif()
