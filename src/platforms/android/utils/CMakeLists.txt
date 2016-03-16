list(
  APPEND ANDROID_DIAGNOSTICS_SRCS
  ${CMAKE_CURRENT_SOURCE_DIR}/test_android_hardware_sanity.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/patterns.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/android_graphics_region_factory.cpp
)

include_directories(
  ${PROJECT_SOURCE_DIR}/include/server
  ${PROJECT_SOURCE_DIR}/include/client
  ${PROJECT_SOURCE_DIR}/include/platform
  ${PROJECT_SOURCE_DIR}/include/test
  ${PROJECT_SOURCE_DIR}/tests/include
  ${PROJECT_SOURCE_DIR}/src/include/server
  ${PROJECT_SOURCE_DIR}/examples
  ${PROJECT_SOURCE_DIR}/src/platforms/android/include
)

add_executable(
  mir_android_diagnostics
  ${ANDROID_DIAGNOSTICS_SRCS})

target_link_libraries(
  mir_android_diagnostics
  mirserver
  mirdraw
  mir-test-static
  mir-test-framework-static
  ${LIBHARDWARE_LIBRARIES}
)

mir_add_wrapped_executable(mir_demo_standalone_render_overlays
  render_overlays.cpp
)

target_link_libraries(mir_demo_standalone_render_overlays
  mirserver
  mircommon
)

install(PROGRAMS ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/mir_android_diagnostics
  DESTINATION ${CMAKE_INSTALL_BINDIR}
)
