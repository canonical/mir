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

if (WLCS_VERSION VERSION_LESS_EQUAL "1.8.1")
  set(WLCS_BAD_BUFFER_TEST_BAD ON)
endif ()

if (MIR_SIGBUS_HANDLER_ENVIRONMENT_BROKEN OR WLCS_BAD_BUFFER_TEST_BAD)
  list(APPEND EXPECTED_FAILURES BadBufferTest.test_truncated_shm_file)
endif ()

set(BROKEN_TESTS
  ClientSurfaceEventsTest.buffer_release # Has been removed upstream

  Anchor/LayerSurfaceLayoutTest.surfaces_with_exclusive_zone_set_to_negative_one_do_not_respect_other_exclusive_zones/*

  ExtImageCopyCaptureTest.cursor_session_captures_pointer_image  # TODO: flaky https://github.com/canonical/mir/issues/4775

  # The following tests are expected to fail due to the fact that we don't yet support subsurface commits that are not
  # synchronized with their parent surface. They also get skipped with the current release of wlcs, and so cannot be
  # marked as expected failures, but we should be able to remove them from this list once wlcs is released.
  TouchInputSubsurfaces/SubsurfaceMultilevelTest.subsurface_moves_after_both_sync_parent_and_grandparent_commit/0
  TouchInputSubsurfaces/SubsurfaceMultilevelTest.subsurface_with_desync_parent_moves_when_only_parent_committed/0
  TouchInputSubsurfaces/SubsurfaceTest.desync_subsurface_moves_when_only_parent_committed/0
  TouchInputSubsurfaces/SubsurfaceTest.one_subsurface_to_another_fallthrough/0
  TouchInputSubsurfaces/SubsurfaceTest.place_above_simple/0
  TouchInputSubsurfaces/SubsurfaceTest.place_below_simple/0
  TouchInputSubsurfaces/SubsurfaceTest.pointer_input_correctly_offset_for_subsurface/0
  TouchInputSubsurfaces/SubsurfaceTest.subsurface_does_not_move_when_parent_not_committed/0
  TouchInputSubsurfaces/SubsurfaceTest.subsurface_extends_parent_input_region/0
  TouchInputSubsurfaces/SubsurfaceTest.subsurface_moves_out_from_under_input_device/0
  TouchInputSubsurfaces/SubsurfaceTest.subsurface_moves_under_input_device_once/0
  TouchInputSubsurfaces/SubsurfaceTest.subsurface_moves_under_input_device_twice/0
  TouchInputSubsurfaces/SubsurfaceTest.sync_subsurface_moves_when_only_parent_committed/0
)

if (MIR_RUN_WLCS_TESTS)
  set(TIMEOUT 600)
  if (CMAKE_SYSTEM_PROCESSOR MATCHES "riscv64")
    # The Launchpad riscv builders are sufficiently slow that some of our tests hit
    # the default 600sec timeout.
    #
    # Bump this up to 3600 for riscv only; for other architectures, leave the lower
    # threshold so that real deadlocks can fail faster.
    set(TIMEOUT 3600)
  endif()

  mir_discover_external_gtests(
    NAME wlcs
    COMMAND ${WLCS_BINARY}
    ARGS ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/miral_wlcs_integration.so
    EXPECTED_FAILURES ${EXPECTED_FAILURES}
    BROKEN_TESTS ${BROKEN_TESTS}
    TIMEOUT ${TIMEOUT}
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
