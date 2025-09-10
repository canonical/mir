cmake_minimum_required(VERSION 3.16)

project(demo-mir-compositor)

set(CMAKE_CXX_STANDARD 23)

include(FindPkgConfig)
pkg_check_modules(MIRAL miral REQUIRED)
pkg_check_modules(XKBCOMMON xkbcommon REQUIRED)

add_executable(demo-mir-compositor main.cpp)

target_include_directories(demo-mir-compositor PUBLIC SYSTEM ${MIRAL_INCLUDE_DIRS})
target_link_libraries(demo-mir-compositor ${MIRAL_LDFLAGS})
target_link_libraries(demo-mir-compositor ${XKBCOMMON_LIBRARIES})
