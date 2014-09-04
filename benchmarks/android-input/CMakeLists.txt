project(android-input-perf)

set(SOURCES
 input_reader_perf.cpp
 )

include_directories(
  ${Mir_SOURCE_DIR}/tests/include
  ${Mir_SOURCE_DIR}/include/platform
)

add_executable(input-reader-perf ${SOURCES})

target_link_libraries(
  input-reader-perf

  android-input
  mir-test-doubles
  mirserver
 )

uses_android_input(input-reader-perf)
