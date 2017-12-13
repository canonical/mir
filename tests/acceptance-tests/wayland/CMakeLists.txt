include(ExternalProject)

ExternalProject_Add(
  wlcs PREFIX ./wlcs
  SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/wlcs
  CMAKE_ARGS
    -DCMAKE_CXX_COMPILER_WORKS=1
    -DCMAKE_C_FLAGS='${CMAKE_C_FLAGS}'
    -DCMAKE_C_COMPILER='${CMAKE_C_COMPILER}'
    -DCMAKE_CXX_FLAGS='${CMAKE_CXX_FLAGS}'
    -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
    -DCMAKE_LIBRARY_OUTPUT_DIRECTORY=${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
  BUILD_BYPRODUCTS ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libwlcs.so
  INSTALL_COMMAND true
  BUILD_ALWAYS TRUE)

add_library(libwlcs SHARED IMPORTED)
set_target_properties(libwlcs PROPERTIES IMPORTED_LOCATION ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libwlcs.so)
add_dependencies(libwlcs wlcs)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/wlcs/include)

mir_add_wrapped_executable(
  mir_wlcs_tests

  wlcs/example/mir_integration.cpp
)

target_link_libraries(
  mir_wlcs_tests

  libwlcs
  mirserver
  mir-test-assist
  mir-test-doubles-platform-static
)

if (MIR_RUN_ACCEPTANCE_TESTS)
  mir_discover_tests_with_fd_leak_detection(mir_wlcs_tests)
endif (MIR_RUN_ACCEPTANCE_TESTS)
