if (MIR_BUILD_PLATFORM_GBM_KMS)
  add_subdirectory(gbm-kms/)
endif()

if (MIR_BUILD_PLATFORM_EGLSTREAM_KMS)
  add_subdirectory(eglstream-kms)
endif()

if (MIR_BUILD_PLATFORM_X11)
  add_subdirectory(x11)
endif()

#add_subdirectory(virtual)

set(UNIT_TEST_SOURCES
  ${UNIT_TEST_SOURCES}
#  ${CMAKE_CURRENT_SOURCE_DIR}/test_rendering_platform.cpp
#  ${CMAKE_CURRENT_SOURCE_DIR}/test_rendering_platform.h
)

set(UNIT_TEST_SOURCES ${UNIT_TEST_SOURCES} PARENT_SCOPE)
