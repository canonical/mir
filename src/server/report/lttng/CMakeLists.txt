include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${LTTNG_UST_INCLUDE_DIRS})

# Don't treat missing-field-initializers as an error, since
# the LTTng macros contain code that triggers this (but it's
# harmless; it concerns a padding field)
add_compile_options(
  -Wno-error=missing-field-initializers
  -Wno-error=unused-function
  -Wno-error=unused-parameter
)

# Add USDT hooks to all LTTNG tracepoints
#
# This might be gnarly; this tweaks an LTTNG header to enable its
# USDT integration. The integration only involves #include-ing a
# SystemTap header and calling a macro from it in the LTTNG macro,
# but there aren't guarantees this won't change in future.
add_definitions(  # Drop in favour of add_compile_definitions when we drop 16.04
  -DLTTNG_UST_HAVE_SDT_INTEGRATION
)

# Using LTO on the lttng DSO causes a gcc ICE.
# Since LTO is reasonably uninteresting for the lttng tracer, disable it.
string(REPLACE "-flto" "" NO_LTO_FLAGS ${CMAKE_C_FLAGS})
set(CMAKE_C_FLAGS ${NO_LTO_FLAGS})

if (LTTNG_UST_VERSION VERSION_GREATER 2.9)
    add_definitions(-DMIR_LTTNG_HAS_VOID_TRACEPOINTS)
endif()

# lttng-ust uses urcu headers which contain code blocks inside expressions 
# this is a gnu extension.
string(REPLACE "-pedantic" "" NO_PEDANTIC_CHECK_FLAGS ${CMAKE_CXX_FLAGS})
if("${CMAKE_CXX_COMPILER}" MATCHES "clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-gnu-statement-expression")
else()
    set(CMAKE_CXX_FLAGS "${NO_PEDANTIC_CHECK_FLAGS}")
endif()

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
