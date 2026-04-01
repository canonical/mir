set(GENERATED_HEADER "${CMAKE_CURRENT_BINARY_DIR}/generated.h")
set(GENERATED_SRC "${CMAKE_CURRENT_BINARY_DIR}/generated.cpp")
set(PROTOCOL_PATH "${CMAKE_CURRENT_SOURCE_DIR}/protocol.xml")
add_custom_command(
  OUTPUT "${GENERATED_HEADER}" "${GENERATED_SRC}"
  VERBATIM
  COMMAND "sh" "-c"
  "${MIR_WAYLAND_GENERATOR_EXECUTABLE} wl_ ${PROTOCOL_PATH} header > ${GENERATED_HEADER}"
  COMMAND "sh" "-c"
  "${MIR_WAYLAND_GENERATOR_EXECUTABLE} wl_ ${PROTOCOL_PATH} source > ${GENERATED_SRC}"
  DEPENDS mir_wayland_generator "${PROTOCOL_PATH}"
)
add_custom_target(wayland_generator_test_generated_files ALL DEPENDS "${GENERATED_SRC}" "${GENERATED_HEADER}")

set(EXPECTED_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/expected.h")
set(EXPECTED_SRC "${CMAKE_CURRENT_SOURCE_DIR}/expected.cpp")
set(TEST_SCRIPT "${CMAKE_BINARY_DIR}/bin/test_wayland_generator.sh")
configure_file("test_wayland_generator.sh.in" "${TEST_SCRIPT}" @ONLY)
configure_file("update_wayland_generator_test.sh.in" "${CMAKE_BINARY_DIR}/bin/update_wayland_generator_test.sh" @ONLY)

mir_add_test(NAME test_wayland_generator COMMAND "${TEST_SCRIPT}")
