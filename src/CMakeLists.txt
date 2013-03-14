set(MIR_GENERATED_INCLUDE_DIRECTORIES)
add_subdirectory(shared/)
include_directories(${MIR_GENERATED_INCLUDE_DIRECTORIES})

add_subdirectory(server/)
add_subdirectory(client/)
add_subdirectory(accessory/)

set(
  MIR_GENERATED_INCLUDE_DIRECTORIES
  ${MIR_GENERATED_INCLUDE_DIRECTORIES}
  PARENT_SCOPE)
