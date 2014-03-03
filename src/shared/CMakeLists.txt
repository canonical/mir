set(MIR_GENERATED_INCLUDE_DIRECTORIES)
set(MIR_COMMON_PLATFORM_LIBRARIES)
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
add_subdirectory(testdraw)

set(
  MIR_COMMON_PLATFORM_LIBRARIES
  ${MIR_COMMON_PLATFORM_LIBRARIES}
  PARENT_SCOPE)

set(PREFIX "${CMAKE_INSTALL_PREFIX}")
set(EXEC_PREFIX "${CMAKE_INSTALL_PREFIX}")
set(LIBDIR "${CMAKE_INSTALL_FULL_LIBDIR}")
set(INCLUDEDIR "${CMAKE_INSTALL_PREFIX}/include/mircommon")

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/mircommon.pc.in
  ${CMAKE_CURRENT_BINARY_DIR}/mircommon.pc
)

set(
  MIR_GENERATED_INCLUDE_DIRECTORIES
  ${MIR_GENERATED_INCLUDE_DIRECTORIES}
  PARENT_SCOPE)

install(
  DIRECTORY ${CMAKE_SOURCE_DIR}/include/shared/mir ${CMAKE_SOURCE_DIR}/include/shared/mir_toolkit
  DESTINATION "include/mircommon"
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/mircommon.pc
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)
