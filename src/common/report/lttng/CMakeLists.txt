join_paths(MIR_TRACEPOINT_LIB_INSTALL_PATH "${CMAKE_INSTALL_PREFIX}" "${MIR_TRACEPOINT_LIB_INSTALL_DIR}")
add_compile_definitions(MIR_TRACEPOINT_LIB_INSTALL_PATH="${MIR_TRACEPOINT_LIB_INSTALL_PATH}")

add_library(mirsharedlttng OBJECT

  tracepoint_provider.cpp
)

list(APPEND MIR_COMMON_SOURCES
  $<TARGET_OBJECTS:mirsharedlttng>
)

set(MIR_COMMON_SOURCES ${MIR_COMMON_SOURCES} PARENT_SCOPE)