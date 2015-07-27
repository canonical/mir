add_subdirectory(common/)

if (MIR_BUILD_PLATFORM_MESA_KMS)
  add_subdirectory(kms/)
endif()
if (MIR_BUILD_PLATFORM_MESA_X11)
  add_subdirectory(x11/)
endif()
