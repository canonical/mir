include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${LTTNG_UST_INCLUDE_DIRS})

add_library(
  mirclientlttngstatic STATIC

  client_tracepoint_provider.cpp
  rpc_report.cpp
)

add_library(
  mirclientlttng SHARED

  rpc_report_tp.c
)

# Don't treat missing-field-initializers as an error, since
# the LTTng macros contain code that triggers this (but it's
# harmless; it concerns a padding field)
set_target_properties(mirclientlttngstatic PROPERTIES
  COMPILE_FLAGS "-Wno-error=missing-field-initializers -Wno-error=unused-function"
)

set_target_properties(mirclientlttng PROPERTIES
  COMPILE_FLAGS "-Wno-error=unused-function"
)

target_link_libraries(
  mirclientlttngstatic

  mirsharedlttng
  -ldl
)

target_link_libraries(
  mirclientlttng

  ${LTTNG_UST_LIBRARIES}
)

install(TARGETS mirclientlttng
  LIBRARY DESTINATION ${MIR_TRACEPOINT_LIB_INSTALL_DIR}
)
