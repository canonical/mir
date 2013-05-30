include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${LTTNG_UST_INCLUDE_DIRS})

set(
  LTTNG_SOURCES

  tracepoint_provider.cpp
  message_processor_report.cpp
  input_report.cpp
)

add_library(
  mirlttng STATIC

  ${LTTNG_SOURCES}
)

set(
  MIR_SERVER_LTTNG_SOURCES

  message_processor_report_tp.c
  input_report_tp.c
)

add_library(
  mirserverlttng SHARED

  ${MIR_SERVER_LTTNG_SOURCES}
)

# Don't treat missing-field-initializers as an error, since
# the LTTng macros contain code that triggers this (but it's
# harmless; it concerns a padding field)
SET_TARGET_PROPERTIES(
  mirlttng

  PROPERTIES
  COMPILE_FLAGS "-Wno-error=missing-field-initializers"
)

target_link_libraries(
  mirlttng

  -ldl
)

target_link_libraries(
  mirserverlttng

  ${LTTNG_UST_LIBRARIES}
)

install(TARGETS mirserverlttng
  LIBRARY DESTINATION lib
)
