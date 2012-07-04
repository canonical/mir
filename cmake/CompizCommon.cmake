cmake_minimum_required (VERSION 2.6)
# Create target to discover tests
function (compiz_discover_tests EXECUTABLE)

    add_custom_command (OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/test_${EXECUTABLE}_wrapper.cpp
			COMMAND echo "int main (void) { return 0; }" >> ${CMAKE_CURRENT_BINARY_DIR}/test_${EXECUTABLE}_wrapper.cpp
			WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
			VERBATIM)

    add_dependencies (${EXECUTABLE}
              compiz_discover_gtest_tests)

    add_custom_command (TARGET ${EXECUTABLE}
			POST_BUILD
			COMMAND ${CMAKE_CURRENT_BINARY_DIR}/${EXECUTABLE} --gtest_list_tests | ${CMAKE_BINARY_DIR}/compiz_gtest/compiz_discover_gtest_tests ${CMAKE_CURRENT_BINARY_DIR}/${EXECUTABLE}
			WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
			COMMENT "Discovering Tests in ${EXECUTABLE}"
			DEPENDS 
			VERBATIM)
endfunction ()

