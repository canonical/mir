set(MIR_GENERATED_INCLUDE_DIRECTORIES)
set(MIR_COMMON_EXTRA_INCLUDE)
set(MIR_COMMON_LIBRARIES)
set(MIR_COMMON_SOURCES)
set(MIR_COMMON_REFERENCES)
add_subdirectory(protobuf/)

if (MIR_BUILD_PLATFORM_ANDROID)
  add_subdirectory(graphics/android)
endif()

add_subdirectory(geometry)
add_subdirectory(input)
add_subdirectory(logging)
add_subdirectory(report/lttng)
add_subdirectory(env)
add_subdirectory(sharedlibrary)
add_subdirectory(thread)
add_subdirectory(testdraw)

set(PREFIX "${CMAKE_INSTALL_PREFIX}")
set(EXEC_PREFIX "${CMAKE_INSTALL_PREFIX}")
set(LIBDIR "${CMAKE_INSTALL_FULL_LIBDIR}")
set(INCLUDEDIR "${CMAKE_INSTALL_PREFIX}/include/mircommon")

set_property(SOURCE report/lttng/tracepoint_provider.cpp
  APPEND PROPERTY COMPILE_DEFINITIONS 
  MIR_TRACEPOINT_LIB_INSTALL_PATH="${CMAKE_INSTALL_PREFIX}/${MIR_TRACEPOINT_LIB_INSTALL_DIR}")

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/mircommon.pc.in
  ${CMAKE_CURRENT_BINARY_DIR}/mircommon.pc
)

set(
  MIR_GENERATED_INCLUDE_DIRECTORIES
  ${MIR_GENERATED_INCLUDE_DIRECTORIES}
  PARENT_SCOPE)

include_directories(${MIR_COMMON_EXTRA_INCLUDE})

add_library(mircommon
  SHARED
  ${MIR_COMMON_SOURCES}
)

target_link_libraries(mircommon
  -Wl,-whole-archive    ${MIR_COMMON_LIBRARIES}
  -Wl,-no-whole-archive ${MIR_COMMON_REFERENCES}
)

# TODO we need a place to manage ABI and related versioning but use this as placeholder
set(MIR_SHARED_ABI 1)

set_target_properties(mircommon

  PROPERTIES
  SOVERSION ${MIR_SHARED_ABI}
)

install(TARGETS mircommon LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

install(
  DIRECTORY ${CMAKE_SOURCE_DIR}/include/shared/mir ${CMAKE_SOURCE_DIR}/include/shared/mir_toolkit
  DESTINATION "include/mircommon"
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/mircommon.pc
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)
