# Check if dpkg-buildflags is available and adjust cmake buildflags
find_program(DPKG_BUILDFLAGS dpkg-buildflags)

if (DPKG_BUILDFLAGS)
  message(STATUS "dpkg-buildflags available, adjusting compiler flags.")
  #dpkg-buildflags is available, adjust cmake buildflags now.
  execute_process(
    COMMAND ${DPKG_BUILDFLAGS} "--get" "CFLAGS"
    OUTPUT_VARIABLE DPKG_BUILDFLAGS_CFLAGS
    OUTPUT_STRIP_TRAILING_WHITESPACE
    )

  execute_process(
    COMMAND ${DPKG_BUILDFLAGS} "--get" "CPPFLAGS"
    OUTPUT_VARIABLE DPKG_BUILDFLAGS_CPPFLAGS
    OUTPUT_STRIP_TRAILING_WHITESPACE
    )

  execute_process(
    COMMAND ${DPKG_BUILDFLAGS} "--get" "CXXFLAGS"
    OUTPUT_VARIABLE DPKG_BUILDFLAGS_CXXFLAGS
    OUTPUT_STRIP_TRAILING_WHITESPACE
    )

  execute_process(
    COMMAND ${DPKG_BUILDFLAGS} "--get" "LDFLAGS"
    OUTPUT_VARIABLE DPKG_BUILDFLAGS_LDFLAGS
    OUTPUT_STRIP_TRAILING_WHITESPACE
    )

  message(STATUS "DPKG_BUILDFLAGS_CFLAGS: " ${DPKG_BUILDFLAGS_CFLAGS})
  message(STATUS "DPKG_BUILDFLAGS_CPPFLAGS: " ${DPKG_BUILDFLAGS_CPPFLAGS})
  message(STATUS "DPKG_BUILDFLAGS_CXXFLAGS: " ${DPKG_BUILDFLAGS_CXXFLAGS})
  message(STATUS "DPKG_BUILDFLAGS_LDFLAGS: " ${DPKG_BUILDFLAGS_LDFLAGS})

  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${DPKG_BUILDFLAGS_CFLAGS}")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${DPKG_BUILDFLAGS_CXXFLAGS}")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${DPKG_BUILDFLAGS_LDFLAGS}")
  add_definitions("${DPKG_BUILDFLAGS_CPPFLAGS}")
else()
  message(WARNING "Could not find dpkg-buildflags, not building with packaging setup C/C++/LD-Flags.")
endif(DPKG_BUILDFLAGS)
