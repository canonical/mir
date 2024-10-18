# This needs to change whenever the ABI between the platform
# modules and the server changes in an incompatible way.
# This ABI is much smaller than the full libmirplatform ABI.
#
# TODO: Add an extra driver-ABI check target.
set(MIR_SERVER_INPUT_PLATFORM_ABI 10)
set(MIR_SERVER_INPUT_PLATFORM_STANZA_VERSION 0.27)
set(MIR_SERVER_INPUT_PLATFORM_ABI ${MIR_SERVER_INPUT_PLATFORM_ABI} PARENT_SCOPE)
set(MIR_SERVER_INPUT_PLATFORM_VERSION "MIR_INPUT_PLATFORM_${MIR_SERVER_INPUT_PLATFORM_STANZA_VERSION}")
set(MIR_SERVER_INPUT_PLATFORM_VERSION ${MIR_SERVER_INPUT_PLATFORM_VERSION} PARENT_SCOPE)
set(MIR_SERVER_GRAPHICS_PLATFORM_ABI 22)
set(MIR_SERVER_GRAPHICS_PLATFORM_STANZA_VERSION 2.18)
set(MIR_SERVER_GRAPHICS_PLATFORM_ABI ${MIR_SERVER_GRAPHICS_PLATFORM_ABI} PARENT_SCOPE)
set(MIR_SERVER_GRAPHICS_PLATFORM_VERSION "MIR_GRAPHICS_PLATFORM_${MIR_SERVER_GRAPHICS_PLATFORM_STANZA_VERSION}")
set(MIR_SERVER_GRAPHICS_PLATFORM_VERSION ${MIR_SERVER_GRAPHICS_PLATFORM_VERSION} PARENT_SCOPE)
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/input_platform_symbols.map.in 
    ${CMAKE_CURRENT_BINARY_DIR}/input_platform_symbols.map)
set(MIR_INPUT_PLATFORM_VERSION_SCRIPT 
    ${CMAKE_CURRENT_BINARY_DIR}/input_platform_symbols.map)
set(MIR_INPUT_PLATFORM_VERSION_SCRIPT ${MIR_INPUT_PLATFORM_VERSION_SCRIPT} PARENT_SCOPE)

set(MIR_SERVER_PLATFORM_PATH
  ${CMAKE_INSTALL_FULL_LIBDIR}/mir/server-platform
)
set(MIR_SERVER_PLATFORM_PATH
  ${MIR_SERVER_PLATFORM_PATH}
  PARENT_SCOPE
)

set(server_common_include_dirs
  ${PROJECT_SOURCE_DIR}/include/renderers/gl
  ${PROJECT_SOURCE_DIR}/include/renderers/sw
  ${PROJECT_SOURCE_DIR}/include/common
  ${CMAKE_CURRENT_SOURCE_DIR}/common/server
)

set(server_symbol_map ${CMAKE_CURRENT_SOURCE_DIR}/common/server/symbols.map)

set(client_common_include_dirs
  ${CMAKE_CURRENT_SOURCE_DIR}/common/client
)

add_subdirectory(common/)

add_subdirectory(renderer-generic-egl/)

if (MIR_BUILD_PLATFORM_GBM_KMS)
    add_subdirectory(gbm-kms/)
endif()

if (MIR_BUILD_PLATFORM_ATOMIC_KMS)
    add_subdirectory(atomic-kms/)
endif()

if (MIR_BUILD_PLATFORM_X11)
    add_subdirectory(x11/)
endif()

if (MIR_BUILD_PLATFORM_EGLSTREAM_KMS)
  add_subdirectory(eglstream-kms)
endif()

if (MIR_BUILD_PLATFORM_WAYLAND)
  add_subdirectory(wayland)
endif()

add_subdirectory(virtual)
add_subdirectory(evdev/)
