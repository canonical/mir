mir_add_wrapped_executable(
  mir_platform_graphics_test_harness

  graphics_platform_test_harness.cpp
)

target_include_directories(
  mir_platform_graphics_test_harness
    PRIVATE
      ${PROJECT_SOURCE_DIR}/src/include/server
      ${PROJECT_SOURCE_DIR}/include/platform
)

target_compile_definitions(
  mir_platform_graphics_test_harness
    PRIVATE
      MIR_SERVER_GRAPHICS_PLATFORM_VERSION="${MIR_SERVER_GRAPHICS_PLATFORM_VERSION}"
)

target_link_libraries(
  mir_platform_graphics_test_harness
    PRIVATE
      ${MIR_SERVER_REFERENCES}
      # We pull in mirserver here not to actually link it, but to use the
      # list of library dependencies associated with the target
      mirserver
      PkgConfig::EGL
      PkgConfig::GLESv2
)
