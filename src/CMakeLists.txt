# We need MIRPLATFORM_ABI in both libmirplatform and the platform implementations.
set(MIRPLATFORM_ABI 11)

# Add the cookie implementation before exposing any APIs
add_subdirectory(cookie/)

# We need MIR_CLIENT_PLATFORM_PATH in both libmirclient and the platform
# implementations
set(MIR_CLIENT_PLATFORM_PATH
  ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/mir/client-platform
)

# Add the platform implementations before exposing any private APIs
add_subdirectory(platforms/)

# the src/include/... directories should be private to the implementation
include_directories(${PROJECT_SOURCE_DIR}/src/include/common)
set(MIR_GENERATED_INCLUDE_DIRECTORIES)
add_subdirectory(common/)
add_subdirectory(protobuf/)
include_directories(${MIR_GENERATED_INCLUDE_DIRECTORIES})

add_subdirectory(platform/)
add_subdirectory(server/)
add_subdirectory(client/)
add_subdirectory(utils/)
add_subdirectory(renderers/)
add_subdirectory(gl/)

add_executable(wrapper wrapper.c)
set_property(TARGET wrapper
             APPEND_STRING PROPERTY COMPILE_FLAGS " -DEXECUTABLE_FORMAT=\\\"%s.bin\\\"")
set_property(TARGET wrapper
             APPEND_STRING PROPERTY COMPILE_FLAGS " -D_DEFAULT_SOURCE -D_BSD_SOURCE")

set(
  MIR_GENERATED_INCLUDE_DIRECTORIES
  ${MIR_GENERATED_INCLUDE_DIRECTORIES}
  PARENT_SCOPE)

set(MIR_SERVER_OBJECTS ${MIR_SERVER_OBJECTS} PARENT_SCOPE)
set(MIR_SERVER_REFERENCES ${MIR_SERVER_REFERENCES} PARENT_SCOPE)
set(MIR_PLATFORM_OBJECTS ${MIR_PLATFORM_OBJECTS} PARENT_SCOPE)
set(MIR_PLATFORM_REFERENCES ${MIR_PLATFORM_REFERENCES} PARENT_SCOPE)
set(MIR_COMMON_OBJECTS ${MIR_COMMON_OBJECTS} PARENT_SCOPE)
set(MIR_COMMON_REFERENCES ${MIR_COMMON_REFERENCES} PARENT_SCOPE)
set(MIR_CLIENT_PLATFORM_PATH ${MIR_CLIENT_PLATFORM_PATH} PARENT_SCOPE)
set(MIR_SERVER_PLATFORM_PATH ${MIR_SERVER_PLATFORM_PATH} PARENT_SCOPE)

# We need the ABI versions in the tests
set(MIR_SERVER_GRAPHICS_PLATFORM_ABI ${MIR_SERVER_GRAPHICS_PLATFORM_ABI} PARENT_SCOPE)
set(MIR_SERVER_INPUT_PLATFORM_ABI ${MIR_SERVER_INPUT_PLATFORM_ABI} PARENT_SCOPE)
set(MIR_CLIENT_PLATFORM_ABI ${MIR_CLIENT_PLATFORM_ABI} PARENT_SCOPE)
set(MIR_INPUT_PLATFORM_VERSION_SCRIPT ${MIR_INPUT_PLATFORM_VERSION_SCRIPT} PARENT_SCOPE)
set(MIR_CLIENT_PLATFORM_VERSION ${MIR_CLIENT_PLATFORM_VERSION} PARENT_SCOPE)
