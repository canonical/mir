set(MIR_GENERATED_INCLUDE_DIRECTORIES)
add_subdirectory(protobuf/)

set(
  MIR_GENERATED_INCLUDE_DIRECTORIES
  ${MIR_GENERATED_INCLUDE_DIRECTORIES}
  PARENT_SCOPE)

# Some headers contain common definitions required for both client and
# server development...
set (MIR_COMMON_HEADERS
  ${CMAKE_SOURCE_DIR}/include/shared/mir_toolkit/common.h
)

# TODO: Find a more elegant include/ structure. Ideally one in which everyting
#       lives under "include/mir/".
install (FILES ${MIR_COMMON_HEADERS} DESTINATION "include/mir_toolkit/")
