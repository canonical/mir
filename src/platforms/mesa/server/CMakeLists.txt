add_subdirectory(common/)

if (MIR_BUILD_PLATFORM_KMS)
  add_subdirectory(kms/)
endif()
