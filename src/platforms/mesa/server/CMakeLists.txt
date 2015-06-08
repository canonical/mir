add_subdirectory(common/)

if (MIR_BUILD_PLATFORM_MESA_KMS)
  add_subdirectory(kms/)
endif()
if (MIR_BUILD_PLATFORM_X)
  add_subdirectory(X/)
endif()
