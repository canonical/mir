include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# Don't treat missing-field-initializers as an error, since
# the LTTng macros contain code that triggers this (but it's
# harmless; it concerns a padding field)
add_compile_options(
  -Wno-error=missing-field-initializers
  -Wno-error=unused-function
  -Wno-error=unused-parameter
)

# lttng relies on a gcc extension to support backward-compatible code, So...
check_cxx_compiler_flag(-Wno-gnu-zero-variadic-macro-arguments MIR_COMPILE_WITH_W_NO_GNU_ZERO_VARIADIC_MACRO_ARGUMENTS)
if ("${CMAKE_CXX_COMPILER}" MATCHES ".*clang.*" AND MIR_COMPILE_WITH_W_NO_GNU_ZERO_VARIADIC_MACRO_ARGUMENTS)
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-gnu-zero-variadic-macro-arguments")
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
  display_report.cpp
  input_report.cpp
  lttng_report_factory.cpp
  scene_report.cpp
  server_tracepoint_provider.cpp
  shared_library_prober_report.cpp
)

add_library(
  mirlttng OBJECT

  ${LTTNG_SOURCES}
)

target_link_libraries(mirlttng
  PUBLIC
    mirplatform
    mircommon
    mircore
)

add_library(mirserverlttng SHARED tracepoints.c)

target_link_libraries(
  mirserverlttng

  PkgConfig::LTTNG_UST
)

install(TARGETS mirserverlttng
  LIBRARY DESTINATION ${MIR_TRACEPOINT_LIB_INSTALL_DIR}
)
