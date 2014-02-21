add_definitions(-DMIR_TRACEPOINT_LIB_INSTALL_PATH="${CMAKE_INSTALL_PREFIX}/${MIR_TRACEPOINT_LIB_INSTALL_DIR}")

add_library(
  mirsharedlttng STATIC

  tracepoint_provider.cpp
)
