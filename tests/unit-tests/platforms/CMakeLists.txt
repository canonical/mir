if (MIR_BUILD_PLATFORM_MESA_KMS OR MIR_BUILD_PLATFORM_MESA_X11)
  add_subdirectory(mesa/)
endif()

if (MIR_BUILD_PLATFORM_EGLSTREAM_KMS)
  add_subdirectory(eglstream-kms)
endif()

set(UNIT_TEST_SOURCES ${UNIT_TEST_SOURCES} PARENT_SCOPE)
