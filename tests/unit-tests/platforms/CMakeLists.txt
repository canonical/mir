if (MIR_BUILD_PLATFORM_GBM_KMS)
  add_subdirectory(gbm-kms/)
endif()

if (MIR_BUILD_PLATFORM_EGLSTREAM_KMS)
  add_subdirectory(eglstream-kms)
endif()

if (MIR_BUILD_PLATFORM_X11)
  add_subdirectory(x11)
endif()

add_subdirectory(virtual)

find_program(WAYLAND_SCANNER_EXECUTABLE wayland-scanner)
if (NOT WAYLAND_SCANNER_EXECUTABLE)
  message(FATAL_ERROR "wayland-scanner is required for EGL buffer allocator tests")
endif()
set(dmabuf_protocol ${PROJECT_SOURCE_DIR}/wayland-protocols/linux-dmabuf-unstable-v1.xml)
add_custom_command(
  OUTPUT linux-dmabuf-client.h linux-dmabuf-client.c
  COMMAND ${WAYLAND_SCANNER_EXECUTABLE} client-header ${dmabuf_protocol} linux-dmabuf-client.h
  COMMAND ${WAYLAND_SCANNER_EXECUTABLE} private-code ${dmabuf_protocol} linux-dmabuf-client.c
  DEPENDS ${dmabuf_protocol}
)

mir_add_wrapped_executable(mir_unit_tests_egl_buffer_allocator NOINSTALL
  test_egl_buffer_allocator.cpp
  linux-dmabuf-client.h
  linux-dmabuf-client.c
  ${PROJECT_SOURCE_DIR}/src/platforms/renderer-generic-egl/buffer_allocator.cpp
)
target_include_directories(mir_unit_tests_egl_buffer_allocator PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(mir_unit_tests_egl_buffer_allocator
  mir-test-static
  mir-test-framework-static
  mir-test-doubles-static
  mir-test-doubles-platform-static
  PkgConfig::WAYLAND_CLIENT
  PkgConfig::DRM
)
# Keep platform libraries last so their static dependencies follow their consumers.
# GBM supplies server_platform_common transitively; linking it directly too can
# repeat the archive and cause duplicate symbol errors.
if (MIR_BUILD_PLATFORM_GBM_KMS)
  target_link_libraries(mir_unit_tests_egl_buffer_allocator mirsharedgbmservercommon-static PkgConfig::GBM)
else()
  target_link_libraries(mir_unit_tests_egl_buffer_allocator server_platform_common)
endif()
add_dependencies(mir_unit_tests_egl_buffer_allocator GMock)
set_target_properties(mir_unit_tests_egl_buffer_allocator PROPERTIES ENABLE_EXPORTS TRUE)
if (MIR_RUN_UNIT_TESTS)
  mir_discover_tests_with_fd_leak_detection(mir_unit_tests_egl_buffer_allocator)
endif()

set(UNIT_TEST_SOURCES
  ${UNIT_TEST_SOURCES}
#  ${CMAKE_CURRENT_SOURCE_DIR}/test_rendering_platform.cpp
#  ${CMAKE_CURRENT_SOURCE_DIR}/test_rendering_platform.h
)

set(UNIT_TEST_SOURCES ${UNIT_TEST_SOURCES} PARENT_SCOPE)
