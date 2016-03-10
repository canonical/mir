add_definitions(-DANDROID)
include_directories(SYSTEM ${LIBHARDWARE_INCLUDE_DIRS})
include_directories(
    ${server_common_include_dirs}
    include/
)

add_subdirectory(server/)
add_subdirectory(client/)
add_subdirectory(common/)
add_subdirectory(utils/)
