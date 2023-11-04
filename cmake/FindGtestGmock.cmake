include(FindPackageHandleStandardArgs)

pkg_check_modules(GTEST QUIET gtest>=1.8.0)
pkg_check_modules(GTEST_MAIN QUIET gtest_main>=1.8.0)
if (GTEST_FOUND AND GTEST_MAIN_FOUND)
    # If we can find package configuration for gtest use it!
    set(GTEST_LIBRARY ${GTEST_LIBRARIES})
    set(GTEST_MAIN_LIBRARY ${GTEST_MAIN_LIBRARIES})
    set(GTEST_BOTH_LIBRARIES ${GTEST_LIBRARY} ${GTEST_MAIN_LIBRARY})
else()
    # If we CANNOT find package configuration for gtest we have different hoops for different distros & series
    find_package(GTest)

    if (NOT GTEST_FOUND)
        include(ExternalProject)

        find_path(GTEST_ROOT
            NAMES CMakeLists.txt
            PATHS /usr/src/gtest /usr/src/googletest/googletest/
            DOC "Path to GTest CMake project")

        ExternalProject_Add(GTest PREFIX ./gtest
                SOURCE_DIR ${GTEST_ROOT}
                CMAKE_ARGS
                    -DCMAKE_CXX_COMPILER_WORKS=1
                    -DCMAKE_CXX_FLAGS='${CMAKE_CXX_FLAGS}'
                    -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
                INSTALL_COMMAND true
                BUILD_BYPRODUCTS
                    ${CMAKE_CURRENT_BINARY_DIR}/gtest/src/GTest-build/libgtest.a
                    ${CMAKE_CURRENT_BINARY_DIR}/gtest/src/GTest-build/libgtest_main.a
                    ${CMAKE_CURRENT_BINARY_DIR}/gtest/src/GMock-build/libgmock.a)

        ExternalProject_Get_Property(GTest binary_dir)

        add_library(gtest UNKNOWN IMPORTED)
        set_target_properties(gtest      PROPERTIES IMPORTED_LOCATION ${binary_dir}/libgtest.a)
        add_dependencies(gtest GTest)
        set(GTEST_LIBRARY "gtest")

        add_library(gtest_main UNKNOWN IMPORTED)
        set_target_properties(gtest_main PROPERTIES IMPORTED_LOCATION ${binary_dir}/libgtest_main.a)
        add_dependencies(gtest_main GTest)
        set(GTEST_MAIN_LIBRARY "gtest_main")

        set(GTEST_BOTH_LIBRARIES ${GTEST_LIBRARY} ${GTEST_MAIN_LIBRARY})
        find_path(GTEST_INCLUDE_DIRS NAMES gtest/gtest.h)
        find_package_handle_standard_args(GTest GTEST_LIBRARY GTEST_BOTH_LIBRARIES GTEST_INCLUDE_DIRS)
    endif()
endif()

pkg_check_modules(GMOCK QUIET gmock)

if (GMOCK_FOUND)
    add_custom_target(GMock)
else()
    find_file(GMOCK_SOURCE
            NAMES gmock-all.cc
            DOC "GMock source"
            PATHS /usr/src/googletest/googlemock/src/ /usr/src/gmock/ /usr/src/gmock/src)

    if (EXISTS ${GMOCK_SOURCE})
        find_path(GMOCK_INCLUDE_DIR gmock/gmock.h PATHS /usr/src/googletest/googlemock/include)

        add_library(GMock STATIC ${GMOCK_SOURCE})
        target_link_libraries(GMock ${GTEST_LIBRARY})

        if (EXISTS /usr/src/googletest/googlemock/src)
            set_source_files_properties(${GMOCK_SOURCE} PROPERTIES COMPILE_FLAGS "-I/usr/src/googletest/googlemock")
        endif()

        if (EXISTS /usr/src/gmock/src)
            set_source_files_properties(${GMOCK_SOURCE} PROPERTIES COMPILE_FLAGS "-I/usr/src/gmock")
        endif()

        find_package_handle_standard_args(GMock DEFAULT_MSG GMOCK_INCLUDE_DIR)

        set(GMOCK_LIBRARY GMock)
    else()
        # Assume gmock is no longer source, we'll find out soon enough if that's wrong
        add_custom_target(GMock)
        string(REPLACE gtest gmock GMOCK_LIBRARY ${GTEST_LIBRARY})
    endif()

    set(GMOCK_LIBRARIES ${GTEST_BOTH_LIBRARIES} ${GMOCK_LIBRARY})
endif()