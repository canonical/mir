include_directories(${CUKE_INCLUDE_DIRS})

set(BEHAVIOR_TESTING_LINK_LIBRARIES 
  mirserver
  
  mir-test-doubles
  3rd_party
  ${Boost_LIBRARIES}
)

set(MIR_TEST_CUCUMBER_LIB_SRCS
  session_management_context.cpp)

add_library(
  mir-test-cucumber STATIC
  ${MIR_TEST_CUCUMBER_LIB_SRCS})

set(SESSION_MANAGER_BEHAVIOR_TESTING_SOURCES session_manager_steps.cpp)
add_executable(session-management-cucumber-steps ${SESSION_MANAGER_BEHAVIOR_TESTING_SOURCES})

include_directories(${PROJECT_SOURCE_DIR}/include/mir_test_cucumber)

target_link_libraries(
  mir-test-cucumber
  ${BEHAVIOR_TESTING_LINK_LIBRARIES}
)

target_link_libraries(
  session-management-cucumber-steps
  mir-test-cucumber

  ${BEHAVIOR_TESTING_LINK_LIBRARIES}
  ${CUKE_LIBRARIES}
  ${GTEST_BOTH_LIBRARIES}
)
