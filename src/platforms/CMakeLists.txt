# This needs to change whenever the ABI between the platform
# modules and the server changes in an incompatible way.
# This ABI is much smaller than the full libmirplatform ABI.
#
# TODO: Add an extra driver-ABI check target.
set(MIR_SERVER_INPUT_PLATFORM_ABI 2)
set(MIR_SERVER_INPUT_PLATFORM_ABI ${MIR_SERVER_INPUT_PLATFORM_ABI} PARENT_SCOPE)
set(MIR_SERVER_INPUT_PLATFORM_VERSION "MIR_INPUT_PLATFORM_${MIR_SERVER_INPUT_PLATFORM_ABI}")
set(MIR_SERVER_INPUT_PLATFORM_VERSION ${MIR_SERVER_INPUT_PLATFORM_VERSION} PARENT_SCOPE)
set(MIR_SERVER_GRAPHICS_PLATFORM_ABI 4)
set(MIR_SERVER_GRAPHICS_PLATFORM_ABI ${MIR_SERVER_GRAPHICS_PLATFORM_ABI} PARENT_SCOPE)
set(MIR_SERVER_GRAPHICS_PLATFORM_VERSION "MIR_GRAPHICS_PLATFORM_${MIR_SERVER_GRAPHICS_PLATFORM_ABI}")
set(MIR_SERVER_GRAPHICS_PLATFORM_VERSION ${MIR_SERVER_GRAPHICS_PLATFORM_VERSION} PARENT_SCOPE)

set(MIR_SERVER_PLATFORM_PATH
  ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/mir/server-platform
)
set(MIR_SERVER_PLATFORM_PATH
  ${MIR_SERVER_PLATFORM_PATH}
  PARENT_SCOPE
)

set(MIR_CLIENT_PLATFORM_ABI 3)
set(MIR_CLIENT_PLATFORM_ABI ${MIR_CLIENT_PLATFORM_ABI} PARENT_SCOPE)
set(MIR_CLIENT_PLATFORM_VERSION "MIR_CLIENT_PLATFORM_${MIR_CLIENT_PLATFORM_ABI}")
set(MIR_CLIENT_PLATFORM_VERSION ${MIR_CLIENT_PLATFORM_VERSION} PARENT_SCOPE)

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

if (MIR_BUILD_PLATFORM_MESA_KMS OR MIR_BUILD_PLATFORM_MESA_X11)
    add_subdirectory(mesa/)
endif()

if (MIR_BUILD_PLATFORM_ANDROID)
  add_subdirectory(android/)
endif()

  add_subdirectory(evdev/)
