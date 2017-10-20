include(FindPackageHandleStandardArgs)

find_package(GTest)

if (NOT GTEST_FOUND)
    include(ExternalProject)

    find_path(GTEST_ROOT
        NAMES CMakeLists.txt
        PATHS /usr/src/gtest /usr/src/googletest/googletest/
        DOC "Path to GTest CMake project")

    ExternalProject_Add(gtest PREFIX ./gtest
            SOURCE_DIR ${GTEST_ROOT}
            CMAKE_ARGS
                -DCMAKE_CXX_FLAGS='${CMAKE_CXX_FLAGS}'
                -DCMAKE_CXX_COMPILER_WORKS=1
                -DCMAKE_C_COMPILER_WORKS=1
                -DCMAKE_FIND_ROOT_PATH=${CMAKE_FIND_ROOT_PATH}
                -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
                -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
                -DCMAKE_FIND_ROOT_PATH=${CMAKE_FIND_ROOT_PATH}
            INSTALL_COMMAND true)

    set(GTEST_LIBRARY "-lgtest")
    set(GTEST_MAIN_LIBRARY "-lgtest_main")
    set(GTEST_BOTH_LIBRARIES ${GTEST_LIBRARY} ${GTEST_MAIN_LIBRARY})
    set(GTEST_DEPENDENCIES "gtest")
    set(GTEST_FOUND TRUE)
    find_path(GTEST_INCLUDE_DIRS NAMES gtest/gtest.h)
    link_directories(${CMAKE_CURRENT_BINARY_DIR}/gtest/src/gtest-build)
    find_package_handle_standard_args(GTest GTEST_LIBRARY GTEST_BOTH_LIBRARIES GTEST_INCLUDE_DIRS)
endif()

find_file(GMOCK_SOURCE
        NAMES gmock-all.cc
        DOC "GMock source"
        PATHS /usr/src/googletest/googlemock/src/ /usr/src/gmock/)

message(STATUS "GMOCK_SOURCE=${GMOCK_SOURCE}")

find_path(GMOCK_INCLUDE_DIR gmock/gmock.h)

add_library(GMock STATIC ${GMOCK_SOURCE})

if (EXISTS /usr/src/googletest/googlemock)
    set_source_files_properties(${GMOCK_SOURCE} PROPERTIES COMPILE_FLAGS "-I/usr/src/googletest/googlemock")
endif()

if (TARGET gtest)
    add_dependencies(GMock gtest)
endif()

find_package_handle_standard_args(GMock DEFAULT_MSG GMOCK_INCLUDE_DIR)

set(GMOCK_LIBRARY GMock)
set(GMOCK_LIBRARIES ${GTEST_BOTH_LIBRARIES} ${GMOCK_LIBRARY})
