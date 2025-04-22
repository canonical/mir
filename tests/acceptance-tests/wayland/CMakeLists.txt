pkg_check_modules(WLCS REQUIRED IMPORTED_TARGET wlcs)

add_library(
  miral_wlcs_integration MODULE

  miral_integration.cpp
)

target_link_libraries(
  miral_wlcs_integration

  mir-test-assist
  PkgConfig::WAYLAND_CLIENT
)

target_include_directories(
  miral_wlcs_integration

  PRIVATE
    PkgConfig::WLCS
)

set(SYMBOL_FILE ${CMAKE_CURRENT_SOURCE_DIR}/symbol.map)

set_target_properties(
  miral_wlcs_integration PROPERTIES

  PREFIX ""
  LINK_FLAGS "-Wl,--version-script,${SYMBOL_FILE}"
  LINK_DEPENDS ${SYMBOL_FILE}
)

pkg_get_variable(WLCS_BINARY wlcs test_runner)

if(cmake_build_type_lower MATCHES "addresssanitizer")
  set(WLCS_BINARY ${WLCS_BINARY}.asan)
elseif(cmake_build_type_lower MATCHES "threadsanitizer")
  set(WLCS_BINARY ${WLCS_BINARY}.tsan)
elseif(cmake_build_type_lower MATCHES "ubsanitizer")
  set(WLCS_BINARY ${WLCS_BINARY}.ubsan)
endif()

# Pull list of expected failures
file(STRINGS
  ${CMAKE_SOURCE_DIR}/tests/acceptance-tests/wayland/expected_wlcs_failures.list
  RAW_EXPECTED_FAILURES
)

foreach(line IN ITEMS ${RAW_EXPECTED_FAILURES})
  # Strip any comments
  string(REGEX REPLACE "#.*" "" filtered_line ${line})
  # If the line is non-empty...
  if(filtered_line)
    # Strip any whitespace...
    string(STRIP ${filtered_line} filtered_line)
    string(LENGTH ${filtered_line} line_length)
    if (${line_length} GREATER "0")
      # ...and if it's still non-empty, add it to the list of expected failures
      list(APPEND EXPECTED_FAILURES ${filtered_line})
    endif()
  endif()
endforeach()

if (MIR_SIGBUS_HANDLER_ENVIRONMENT_BROKEN)
  list(APPEND EXPECTED_FAILURES BadBufferTest.test_truncated_shm_file)
endif ()

set(BROKEN_TESTS
  ClientSurfaceEventsTest.buffer_release # Has been removed upstream

  Anchor/LayerSurfaceLayoutTest.surfaces_with_exclusive_zone_set_to_negative_one_do_not_respect_other_exclusive_zones/*
)

if (MIR_RUN_WLCS_TESTS)
  mir_discover_external_gtests(
    NAME wlcs
    COMMAND ${WLCS_BINARY}
    ARGS ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/miral_wlcs_integration.so
    EXPECTED_FAILURES ${EXPECTED_FAILURES}
    BROKEN_TESTS ${BROKEN_TESTS}
  )
endif()

install(TARGETS miral_wlcs_integration LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/mir)
install(FILES expected_wlcs_failures.list DESTINATION ${CMAKE_INSTALL_DATADIR}/mir)

add_custom_target(
  list_missing_wlcs_xfail_tests

  COMMAND
    ${CMAKE_COMMAND}
      -DWLCS_BINARY=${WLCS_BINARY}
      -DWLCS_INTEGRATION=${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/miral_wlcs_integration.so
      -DTESTS_TO_QUERY="${EXPECTED_FAILURES}"
      -P ${CMAKE_CURRENT_SOURCE_DIR}/print_missing_wlcs_tests.cmake
)
