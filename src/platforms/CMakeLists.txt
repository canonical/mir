set(server_common_include_dirs
  ${PROJECT_SOURCE_DIR}/include/platform
)

# TODO platform implementations shouldn't depend on private APIs
set(server_common_include_dirs
  ${server_common_include_dirs}
  ${PROJECT_SOURCE_DIR}/src/include/common
  ${PROJECT_SOURCE_DIR}/src/include/platform
)

set(server_symbol_map ${CMAKE_CURRENT_SOURCE_DIR}/common/server/symbols.map)

set(client_common_include_dirs
  ${PROJECT_SOURCE_DIR}/include/client
  ${CMAKE_CURRENT_SOURCE_DIR}/common/client
)

# TODO client platform implementations shouldn't depend on private APIs
set(client_common_include_dirs
  ${client_common_include_dirs}
  ${PROJECT_SOURCE_DIR}/src/include/client
  ${PROJECT_SOURCE_DIR}/src/include/common
)

add_subdirectory(common/)

if (MIR_BUILD_PLATFORM_MESA)
    add_subdirectory(mesa/)
endif()

if (MIR_BUILD_PLATFORM_ANDROID)
  add_subdirectory(android/)
endif()
