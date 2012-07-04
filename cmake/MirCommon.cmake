cmake_minimum_required (VERSION 2.6)
# Create target to discover tests
function (mir_discover_tests EXECUTABLE)

    add_dependencies (${EXECUTABLE}
              mir_discover_gtest_tests)

    add_custom_command (TARGET ${EXECUTABLE}
			POST_BUILD
			COMMAND ${CMAKE_CURRENT_BINARY_DIR}/${EXECUTABLE} --gtest_list_tests | ${CMAKE_BINARY_DIR}/mir_gtest/mir_discover_gtest_tests ${CMAKE_CURRENT_BINARY_DIR}/${EXECUTABLE}
			WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
			COMMENT "Discovering Tests in ${EXECUTABLE}"
			DEPENDS 
			VERBATIM)
endfunction ()

