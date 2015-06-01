add_subdirectory(common/)

if (MIR_BUILD_PLATFORM_MESA)
  add_subdirectory(kms/)
endif()
