include_directories(
  ${PROJECT_SOURCE_DIR}/include/platform
)

# TODO platform implementations shouldn't depend on private APIs
include_directories(
  ${PROJECT_SOURCE_DIR}/src/include/common
  ${PROJECT_SOURCE_DIR}/src/include/platform
)

set(symbol_map ${CMAKE_CURRENT_SOURCE_DIR}/symbols.map)

if (MIR_BUILD_PLATFORM_MESA)
    add_subdirectory(mesa/)
endif()

if (MIR_BUILD_PLATFORM_ANDROID)
  add_subdirectory(android/)
endif()
