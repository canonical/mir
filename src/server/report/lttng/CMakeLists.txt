include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${LTTNG_UST_INCLUDE_DIRS})

# Don't treat missing-field-initializers as an error, since
# the LTTng macros contain code that triggers this (but it's
# harmless; it concerns a padding field)
add_compile_options(-Wno-error=missing-field-initializers -Wno-error=unused-function)

set(
  LTTNG_SOURCES

  compositor_report.cpp
  connector_report.cpp
  display_report.cpp
  input_report.cpp
  message_processor_report.cpp
  lttng_report_factory.cpp
  session_mediator_report.cpp
  scene_report.cpp
  server_tracepoint_provider.cpp
  shared_library_prober_report.cpp
)

add_library(
  mirlttng OBJECT

  ${LTTNG_SOURCES}
)

add_library(mirserverlttng SHARED tracepoints.c)

target_link_libraries(
  mirserverlttng

  ${LTTNG_UST_LIBRARIES}
)

install(TARGETS mirserverlttng
  LIBRARY DESTINATION ${MIR_TRACEPOINT_LIB_INSTALL_DIR}
)
