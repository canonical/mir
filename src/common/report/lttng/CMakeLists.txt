join_paths(MIR_TRACEPOINT_LIB_INSTALL_PATH "${CMAKE_INSTALL_PREFIX}" "${MIR_TRACEPOINT_LIB_INSTALL_DIR}")
add_compile_definitions(MIR_TRACEPOINT_LIB_INSTALL_PATH="${MIR_TRACEPOINT_LIB_INSTALL_PATH}")

add_library(mirsharedlttng OBJECT

  tracepoint_provider.cpp
)

target_include_directories(mirsharedlttng
  PUBLIC
    ${PROJECT_SOURCE_DIR}/src/include/common
)
