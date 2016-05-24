include_directories(
  ${PROJECT_SOURCE_DIR}/include/platforms/mesa
  ${PROJECT_SOURCE_DIR}/src/platforms/mesa/server
)

if (MIR_BUILD_PLATFORM_MESA_KMS)
  add_subdirectory(kms/)
endif()

if (MIR_BUILD_PLATFORM_MESA_X11)
  add_subdirectory(x11/)
endif()

add_library(unit_test_graphics_mesa_common OBJECT
  ${CMAKE_CURRENT_SOURCE_DIR}/test_ipc_operations.cpp
)

list(APPEND UNIT_TEST_SOURCES
  $<TARGET_OBJECTS:unit_test_graphics_mesa_common>
)

list(APPEND UMOCK_UNIT_TEST_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/test_drm_helper.cpp
)

set(UNIT_TEST_SOURCES ${UNIT_TEST_SOURCES} PARENT_SCOPE)
set(UMOCK_UNIT_TEST_SOURCES ${UMOCK_UNIT_TEST_SOURCES} PARENT_SCOPE)
