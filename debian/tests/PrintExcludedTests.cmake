cmake_minimum_required(VERSION 3.16)

function(pkg_check_modules)
  # Dummy
endfunction()
function(include_directories)
  # Dummy
endfunction()
function(add_library)
  # Dummy
endfunction()
function(mir_discover_external_gtests)
  # Dummy
endfunction()
function(target_link_libraries)
  # Dummy
endfunction()
function(set_target_properties)
  # Dummy
endfunction()
function(pkg_get_variable)
  # Dummy
endfunction()
function(install)
  # Dummy
endfunction()
function(add_custom_target)
  # Dummy
endfunction()

include(${UPSTREAM_WLCS_TESTCASE_FILE})

string(JOIN "\n" EXPECTED_FAILURE_LIST ${EXPECTED_FAILURES})
message(${EXPECTED_FAILURE_LIST})
