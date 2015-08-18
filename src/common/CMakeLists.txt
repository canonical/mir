add_definitions(-DMIR_LOG_COMPONENT_FALLBACK="mircommon")

set(MIR_GENERATED_INCLUDE_DIRECTORIES)
set(MIR_COMMON_SOURCES)
set(MIR_COMMON_REFERENCES)

if (MIR_BUILD_PLATFORM_ANDROID)
  add_subdirectory(graphics/android)
endif()

add_subdirectory(fd)
add_subdirectory(geometry)
add_subdirectory(logging)
add_subdirectory(report/lttng)
add_subdirectory(env)
add_subdirectory(sharedlibrary)
add_subdirectory(thread)
add_subdirectory(time)
add_subdirectory(dispatch)
add_subdirectory(fatal)

list(APPEND MIR_COMMON_SOURCES
  $<TARGET_OBJECTS:mirtime>
  $<TARGET_OBJECTS:mirfatal>
  $<TARGET_OBJECTS:android-input-common>
  ${CMAKE_CURRENT_SOURCE_DIR}/log.cpp
  )

set(PREFIX "${CMAKE_INSTALL_PREFIX}")
set(EXEC_PREFIX "${CMAKE_INSTALL_PREFIX}")
set(LIBDIR "${CMAKE_INSTALL_FULL_LIBDIR}")
set(INCLUDEDIR "${CMAKE_INSTALL_PREFIX}/include/mircommon")

set(
  MIR_GENERATED_INCLUDE_DIRECTORIES
  ${MIR_GENERATED_INCLUDE_DIRECTORIES}
  PARENT_SCOPE)

add_library(mircommon
  SHARED
  ${MIR_COMMON_SOURCES}
)

target_link_libraries(mircommon
  ${MIR_COMMON_REFERENCES}
)

# TODO we need a place to manage ABI and related versioning but use this as placeholder
set(MIRCOMMON_ABI 5)
set(symbol_map ${CMAKE_CURRENT_SOURCE_DIR}/symbols.map)

set_target_properties(mircommon

  PROPERTIES
  SOVERSION ${MIRCOMMON_ABI}
  LINK_FLAGS "-Wl,--exclude-libs=ALL -Wl,--version-script,${symbol_map}"
)

install(TARGETS mircommon LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

install(
  DIRECTORY ${CMAKE_SOURCE_DIR}/include/common/mir ${CMAKE_SOURCE_DIR}/include/common/mir_toolkit
  DESTINATION "include/mircommon"
)

set(MIR_COMMON_OBJECTS ${MIR_COMMON_SOURCES} PARENT_SCOPE)
set(MIR_COMMON_REFERENCES ${MIR_COMMON_REFERENCES} PARENT_SCOPE)
