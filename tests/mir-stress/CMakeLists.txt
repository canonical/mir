cmake_minimum_required(VERSION 2.6)
project(mir-stress)

include(FindPkgConfig)

set( CMAKE_CXX_FLAGS "-std=c++11 -ggdb")
pkg_check_modules(MIRCLIENT REQUIRED mirclient)
include_directories(${MIRCLIENT_INCLUDE_DIRS} src)
link_directories(${MIRCLIENT_LIBRARY_DIRS})

add_executable(mir-stress src/mir-stress.cpp src/client.cpp src/threading.cpp src/results.cpp)
target_link_libraries(mir-stress ${MIRCLIENT_LIBRARIES})

