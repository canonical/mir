include_directories(
  ${PROJECT_SOURCE_DIR}/include/platforms/mesa
  ${PROJECT_SOURCE_DIR}/src/platforms/mesa/server
)

add_subdirectory(kms-utils/)

if (MIR_BUILD_PLATFORM_MESA_KMS)
  add_definitions(-DMESA_KMS)
  add_subdirectory(kms/)
endif()

if (MIR_BUILD_PLATFORM_MESA_X11)
  add_definitions(-DMESA_X11)
  add_subdirectory(x11/)
endif()
