if (MIR_DISTRO MATCHES "Fedora")
  find_package(GTest REQUIRED)

  set(GMOCK_SOURCE /usr/src/gmock/gmock-all.cc)

  find_path(GMOCK_INCLUDE_DIR gmock/gmock.h)
  add_library(GMock STATIC ${GMOCK_SOURCE})

  find_package_handle_standard_args(GMock DEFAULT_MSG GMOCK_INCLUDE_DIR)

  set(GMOCK_LIBRARY GMock)
  set(GMOCK_LIBRARIES ${GTEST_BOTH_LIBRARIES} ${GMOCK_LIBRARY})

  return()
endif()

execute_process(COMMAND "arch" OUTPUT_VARIABLE MIR_HOST_PROCESSOR OUTPUT_STRIP_TRAILING_WHITESPACE)

if ("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "${MIR_HOST_PROCESSOR}")
  find_package(GMock)
  if (GMOCK_FOUND)
    if (NOT TARGET GMock)
      add_custom_target(GMock DEPENDS gmock)
    endif()
  endif()
endif()

if (TARGET GMock)
  return()
endif()

include(ExternalProject)
include(FindPackageHandleStandardArgs)

#
# When cross compiling MIR_CHROOT points to our chroot.
# When not cross compiling, it should be blank to use the host system.
#
set(usr ${MIR_CHROOT}/usr)

if (EXISTS ${usr}/src/googletest)
  set (USING_GOOGLETEST_1_8 TRUE)
  set (GTEST_INSTALL_DIR ${usr}/src/googletest/googletest/include)
else()
  set (GTEST_INSTALL_DIR ${usr}/src/gmock/gtest/include)
endif()

#gtest
find_path(
  GTEST_INCLUDE_DIR gtest/gtest.h
  HINTS ${GTEST_INSTALL_DIR}
)

#gmock
find_path(
  GMOCK_INSTALL_DIR CMakeLists.txt
  HINTS ${usr}/src/googletest ${usr}/src/gmock)
if(${GMOCK_INSTALL_DIR} STREQUAL "GMOCK_INSTALL_DIR-NOTFOUND")
    message(FATAL_ERROR "google-mock package not found")
endif()

find_path(GMOCK_INCLUDE_DIR gmock/gmock.h)

if (USING_GOOGLETEST_1_8)
  set(GMOCK_BASE_BINARY_DIR ${CMAKE_BINARY_DIR}/gmock/libs)
  set(GMOCK_BINARY_DIR ${GMOCK_BASE_BINARY_DIR}/googlemock)
  set(GTEST_BINARY_DIR ${GMOCK_BINARY_DIR}/gtest)
else()
  set(GMOCK_BASE_BINARY_DIR ${CMAKE_BINARY_DIR}/gmock/libs)
  set(GMOCK_BINARY_DIR ${GMOCK_BASE_BINARY_DIR})
  set(GTEST_BINARY_DIR ${GMOCK_BINARY_DIR}/gtest)
endif()

set(GTEST_CXX_FLAGS "-fPIC -D_GNU_SOURCE -D_FILE_OFFSET_BITS=64")
if (cmake_build_type_lower MATCHES "threadsanitizer")
  set(GTEST_CXX_FLAGS "${GTEST_CXX_FLAGS} -fsanitize=thread")
elseif (cmake_build_type_lower MATCHES "ubsanitizer")
  set(GTEST_CXX_FLAGS "${GTEST_CXX_FLAGS} -fsanitize=undefined")
endif()

set(GTEST_CMAKE_ARGS "-DCMAKE_CXX_FLAGS=${GTEST_CXX_FLAGS}")
list(APPEND GTEST_CMAKE_ARGS -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER})
list(APPEND GTEST_CMAKE_ARGS -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER})

if (USING_GOOGLETEST_1_8)
  list(APPEND GTEST_CMAKE_ARGS -DBUILD_GTEST=ON)
endif()

if (cmake_build_type_lower MATCHES "threadsanitizer")
  #Skip compiler check, since if GCC is the compiler, we need to link against -ltsan
  #explicitly; specifying additional linker flags doesn't seem possible for external projects
  list(APPEND GTEST_CMAKE_ARGS -DCMAKE_CXX_COMPILER_WORKS=1)
endif()
if (${CMAKE_CROSSCOMPILING})
  if(DEFINED MIR_CHROOT)
    list(APPEND GTEST_CMAKE_ARGS -DCMAKE_TOOLCHAIN_FILE=${CMAKE_MODULE_PATH}/LinuxCrossCompile.cmake)
  else()
    list(APPEND GTEST_CMAKE_ARGS -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER})
    list(APPEND GTEST_CMAKE_ARGS -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER})
  endif()
endif()

ExternalProject_Add(
    GMock
    #where to build in source tree
    PREFIX ${GMOCK_PREFIX}
    #where the source is external to the project
    SOURCE_DIR ${GMOCK_INSTALL_DIR}
    #forward the compilers to the subproject so cross-arch builds work
    CMAKE_ARGS ${GTEST_CMAKE_ARGS}
    BINARY_DIR ${GMOCK_BASE_BINARY_DIR}

    #we don't need to install, so skip
    INSTALL_COMMAND ""
)

set(GMOCK_LIBRARY ${GMOCK_BINARY_DIR}/libgmock.a)
set(GMOCK_MAIN_LIBRARY ${GMOCK_BINARY_DIR}/libgmock_main.a)
set(GMOCK_BOTH_LIBRARIES ${GMOCK_LIBRARY} ${GMOCK_MAIN_LIBRARY})
set(GTEST_LIBRARY ${GTEST_BINARY_DIR}/libgtest.a)
set(GTEST_MAIN_LIBRARY ${GTEST_BINARY_DIR}/libgtest_main.a)
set(GTEST_BOTH_LIBRARIES ${GTEST_LIBRARY} ${GTEST_MAIN_LIBRARY})
set(GMOCK_LIBRARIES ${GTEST_BOTH_LIBRARIES} ${GMOCK_BOTH_LIBRARIES})

find_package_handle_standard_args(GTest  DEFAULT_MSG
                                    GMOCK_INCLUDE_DIR
                                    GTEST_INCLUDE_DIR)
