project(android-input-perf)

include_directories(
  ${Mir_SOURCE_DIR}/include/test
  ${Mir_SOURCE_DIR}/include/platform
  ${MIR_ANDROID_INCLUDE_DIRECTORIES}
  ${MIR_3RD_PARTY_INCLUDE_DIRECTORIES}
)

mir_add_wrapped_executable(input-reader-perf NOINSTALL
  input_reader_perf.cpp
)

target_link_libraries(
  input-reader-perf

  mir-test-assist
 )

uses_android_input(input-reader-perf)
