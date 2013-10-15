project(android-input-perf)

set(SOURCES
 input_reader_perf.cpp
 )

add_executable(input-reader-perf ${SOURCES})

target_link_libraries(
  input-reader-perf

  android-input
  mir-test-doubles
 )

uses_android_input(input-reader-perf)

install(
 TARGETS input-reader-perf
 RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
