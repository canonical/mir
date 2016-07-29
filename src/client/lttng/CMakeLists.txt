include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${LTTNG_UST_INCLUDE_DIRS})

add_library(
  mirclientlttng-static STATIC

  client_tracepoint_provider.cpp
  rpc_report.cpp
  input_receiver_report.cpp
  shared_library_prober_report.cpp
  perf_report.cpp
)

# Using LTO on the lttng DSO causes a gcc ICE.
# Since LTO is reasonably uninteresting for the lttng tracer, disable it.
string(REPLACE "-flto" "" NO_LTO_FLAGS ${CMAKE_C_FLAGS})
set(CMAKE_C_FLAGS ${NO_LTO_FLAGS})

# lttng-ust uses urcu headers which contain code blocks inside expressions 
# this is a gnu extension.
string(REPLACE "-pedantic" "" NO_PEDANTIC_CHECK_FLAGS ${CMAKE_CXX_FLAGS})
if("${CMAKE_CXX_COMPILER}" MATCHES "clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-gnu-statement-expression")
else()
    set(CMAKE_CXX_FLAGS "${NO_PEDANTIC_CHECK_FLAGS}")
endif()

add_library(mirclientlttng SHARED tracepoints.c)

# Don't treat missing-field-initializers as an error, since
# the LTTng macros contain code that triggers this (but it's
# harmless; it concerns a padding field)
set_target_properties(mirclientlttng-static PROPERTIES
  COMPILE_FLAGS "-Wno-error=missing-field-initializers -Wno-error=unused-function"
)

set_target_properties(mirclientlttng PROPERTIES
  COMPILE_FLAGS "-Wno-error=unused-function"
)

target_link_libraries(
  mirclientlttng-static

  -ldl
)

add_dependencies(mirclientlttng-static mirprotobuf)

target_link_libraries(
  mirclientlttng

  ${LTTNG_UST_LIBRARIES}
)

install(TARGETS mirclientlttng
  LIBRARY DESTINATION ${MIR_TRACEPOINT_LIB_INSTALL_DIR}
)
