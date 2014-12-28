# We need MIRPLATFORM_ABI and MIR_PLATFORM_DRIVER_BINARY in both libmirplatform 
# and the platform implementations.
set(MIRPLATFORM_ABI 5)

set(PLATFORM_DRIVER platform${MIRPLATFORM_ABI}driver)
set(MIR_PLATFORM_DRIVER mir${PLATFORM_DRIVER})
set(MIR_PLATFORM_DRIVER_BINARY lib${MIR_PLATFORM_DRIVER}.so
    CACHE STRING "File name of Mir server platform drivers")
add_definitions(-DMIR_PLATFORM_DRIVER_BINARY="${MIR_PLATFORM_DRIVER_BINARY}")

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
