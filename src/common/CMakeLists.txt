add_definitions(-DMIR_LOG_COMPONENT_FALLBACK="mircommon")

set(MIR_COMMON_SOURCES)
set(MIR_COMMON_REFERENCES)

include_directories(${MIR_GENERATED_INCLUDE_DIRECTORIES})

add_subdirectory(fd)
add_subdirectory(events)
add_subdirectory(logging)
add_subdirectory(report/lttng)
add_subdirectory(env)
add_subdirectory(sharedlibrary)
add_subdirectory(thread)
add_subdirectory(time)
add_subdirectory(dispatch)

list(APPEND MIR_COMMON_SOURCES
  $<TARGET_OBJECTS:mirevents>
  $<TARGET_OBJECTS:mirtime>
  ${CMAKE_CURRENT_SOURCE_DIR}/output_type_names.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/log.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/libname.cpp
  ${PROJECT_SOURCE_DIR}/include/common/mir/libname.h
  ${PROJECT_SOURCE_DIR}/include/common/mir/posix_rw_mutex.h
  posix_rw_mutex.cpp
  edid.cpp
)

set(PREFIX "${CMAKE_INSTALL_PREFIX}")
set(EXEC_PREFIX "${CMAKE_INSTALL_PREFIX}")
set(LIBDIR "${CMAKE_INSTALL_FULL_LIBDIR}")
set(INCLUDEDIR "${CMAKE_INSTALL_PREFIX}/include/mircommon")

set(
  MIR_GENERATED_INCLUDE_DIRECTORIES
  ${MIR_GENERATED_INCLUDE_DIRECTORIES}
  PARENT_SCOPE)

# TODO we need a place to manage ABI and related versioning but use this as placeholder
set(MIRCOMMON_ABI 7)
set(symbol_map ${CMAKE_CURRENT_SOURCE_DIR}/symbols.map)

add_library(mircommon SHARED
  input/mir_input_config.cpp
  input/mir_input_config_serialization.cpp
  input/mir_touchpad_config.cpp
  input/mir_pointer_config.cpp
  input/mir_keyboard_config.cpp
  input/mir_touchscreen_config.cpp
  ${PROJECT_SOURCE_DIR}/include/common/mir/input/mir_input_config.h
  ${PROJECT_SOURCE_DIR}/include/common/mir/input/mir_pointer_config.h
  ${PROJECT_SOURCE_DIR}/include/common/mir/input/mir_touchpad_config.h
  ${PROJECT_SOURCE_DIR}/include/common/mir/input/mir_touchscreen_config.h
  ${PROJECT_SOURCE_DIR}/include/common/mir/input/mir_keyboard_config.h
  ${PROJECT_SOURCE_DIR}/include/common/mir/input/mir_input_config_serialization.h
  ${MIR_COMMON_SOURCES}
)

target_link_libraries(mircommon
  mircapnproto
  mircore
  ${MIR_COMMON_REFERENCES}
)

add_dependencies(mircommon mircapnproto)

set_target_properties(mircommon

  PROPERTIES
  SOVERSION ${MIRCOMMON_ABI}
  LINK_FLAGS "-Wl,--exclude-libs=ALL -Wl,--version-script,${symbol_map}"
  LINK_DEPENDS ${symbol_map}
)

install(TARGETS mircommon LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

install(
  DIRECTORY ${CMAKE_SOURCE_DIR}/include/common/mir
  DESTINATION "include/mircommon"
)

set(LIBDIR "${CMAKE_INSTALL_FULL_LIBDIR}")
set(INCLUDEDIR "${CMAKE_INSTALL_PREFIX}/include")

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/mircommon.pc.in
    ${CMAKE_CURRENT_BINARY_DIR}/mircommon.pc
    @ONLY
)
install(FILES       ${CMAKE_CURRENT_BINARY_DIR}/mircommon.pc    DESTINATION "${CMAKE_INSTALL_FULL_LIBDIR}/pkgconfig")
