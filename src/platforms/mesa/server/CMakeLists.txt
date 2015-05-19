#add_subdirectory(common/)
add_subdirectory(KMS/)

if (MIR_BUILD_PLATFORM_X)
  add_subdirectory(X/)
endif()
