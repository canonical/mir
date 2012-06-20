# Build Google Test and make its headers known
find_package (GTest)
if(NOT GTEST_FOUND)
  # Check for google test and build it locally
  set(GTEST_ROOT_DIR
    "/usr/src/gtest" # Default value, adjustable by user with e.g., ccmake
    CACHE
    PATH
    "Path to Google test srcs"
    )
  
  find_path(GTEST_INCLUDE_DIR gtest/gtest.h)
  
  if(GTEST_INCLUDE_DIR)
    add_subdirectory(${GTEST_ROOT_DIR} gtest)
  endif(GTEST_INCLUDE_DIR)
  
  set(GTEST_BOTH_LIBRARIES gtest gtest_main)
  set(GTEST_FOUND TRUE)
endif(NOT GTEST_FOUND)

# TODO(tvoss): Move to its own FindGoogleMock module
find_library(GMOCK_LIBRARY gmock)
find_library(GMOCK_MAIN_LIBRARY gmock_main)

if (NOT GMOCK_LIBRARY OR NOT GMOCK_MAIN_LIBRARY OR NOT GTEST_FOUND)
  message ("Google Mock and Google Test not found - cannot build tests!")
endif (NOT GMOCK_LIBRARY OR NOT GMOCK_MAIN_LIBRARY OR NOT GTEST_FOUND)

include_directories (${GTEST_INCLUDE_DIRS})

find_package (Threads REQUIRED)

add_executable(test_renderloop test_renderloop.cpp)

target_link_libraries(
  test_renderloop
  ${GTEST_BOTH_LIBRARIES}
  ${GMOCK_LIBRARY}
  ${GMOCK_MAIN_LIBRARY}
  ${CMAKE_THREAD_LIBS_INIT} # Link in pthread. 
  )

gtest_add_tests(test_renderloop "" ${CMAKE_CURRENT_SOURCE_DIR}/test_renderloop.cpp )
