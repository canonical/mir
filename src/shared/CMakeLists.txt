set(MIR_GENERATED_INCLUDE_DIRECTORIES)
add_subdirectory(protobuf/)
add_subdirectory(draw/)

set(
  MIR_GENERATED_INCLUDE_DIRECTORIES
  ${MIR_GENERATED_INCLUDE_DIRECTORIES}
  PARENT_SCOPE)

# Some headers contain common definitions required for both client and
# server development...
set (MIR_COMMON_HEADERS
  ${CMAKE_SOURCE_DIR}/include/shared/mir/common.h
)

install (FILES ${MIR_COMMON_HEADERS} DESTINATION "include/mir/")
