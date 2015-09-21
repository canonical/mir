find_program(VERA_EXECUTABLE vera++)

if(VERA_EXECUTABLE)
  message(STATUS "vera++ available - enabling make target style_check")

  add_custom_target(
    style_check
    COMMAND "find" "${CMAKE_SOURCE_DIR}/include" "-name" "'*.h'" "|" "xargs" "vera++" "-profile" "mir_vera_profile"
    COMMAND "find" "${CMAKE_SOURCE_DIR}/src" "-name" "'*.h'" "|" "xargs" "vera++" "-profile" "mir_vera_profile"
    COMMAND "find" "${CMAKE_SOURCE_DIR}/src" "-name" "'*.cpp'" "|" "xargs" "vera++" "-profile" "mir_vera_profile"
    COMMAND "find" "${CMAKE_SOURCE_DIR}/tests" "-name" "'*.h'" "|" "xargs" "vera++" "-profile" "mir_vera_profile"
    COMMAND "find" "${CMAKE_SOURCE_DIR}/tests" "-name" "'*.cpp'" "|" "xargs" "vera++" "-profile" "mir_vera_profile"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    )
else()
  message(STATUS "vera++ not available - disabling make target style_check")
endif(VERA_EXECUTABLE)
