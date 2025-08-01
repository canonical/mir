pkg_check_modules(XKBCOMMON REQUIRED IMPORTED_TARGET xkbcommon)

add_compile_definitions(MIR_LOG_COMPONENT_FALLBACK="mircommon")

set(MIR_COMMON_SOURCES)
set(MIR_COMMON_REFERENCES)

include_directories(
  ${MIR_GENERATED_INCLUDE_DIRECTORIES}
  ${PROJECT_SOURCE_DIR}/include/common
)

add_subdirectory(events)
add_subdirectory(logging)
add_subdirectory(report/lttng)
add_subdirectory(sharedlibrary)
add_subdirectory(thread)
add_subdirectory(time)
add_subdirectory(dispatch)

list(APPEND MIR_COMMON_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/output_type_names.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/log.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/libname.cpp
  ${PROJECT_SOURCE_DIR}/include/common/mir/libname.h
  ${PROJECT_SOURCE_DIR}/include/common/mir/posix_rw_mutex.h
  posix_rw_mutex.cpp
  event.cpp
  event_printer.cpp
  mir_cursor_api.cpp
  thread_pool_executor.cpp
  linearising_executor.cpp
  immediate_executor.cpp
  ${PROJECT_SOURCE_DIR}/include/common/mir/signal.h
  signal.cpp
)

list(APPEND MIR_COMMON_REFERENCES
  mirevents
  mirtime
)

set(
  MIR_GENERATED_INCLUDE_DIRECTORIES
  ${MIR_GENERATED_INCLUDE_DIRECTORIES}
  PARENT_SCOPE)

# TODO we need a place to manage ABI and related versioning but use this as placeholder
set(MIRCOMMON_ABI 11)
set(symbol_map ${CMAKE_CURRENT_SOURCE_DIR}/symbols.map)

add_library(mircommon SHARED
  input/mir_input_config.cpp
  input/mir_touchpad_config.cpp
  input/mir_pointer_config.cpp
  input/mir_keyboard_config.cpp
  input/mir_touchscreen_config.cpp
  input/input_event.cpp
  input/xkb_mapper.cpp
  input/parameter_keymap.cpp
  input/buffer_keymap.cpp
  ${PROJECT_SOURCE_DIR}/include/common/mir/input/mir_input_config.h
  ${PROJECT_SOURCE_DIR}/include/common/mir/input/mir_pointer_config.h
  ${PROJECT_SOURCE_DIR}/include/common/mir/input/mir_touchpad_config.h
  ${PROJECT_SOURCE_DIR}/include/common/mir/input/mir_touchscreen_config.h
  ${PROJECT_SOURCE_DIR}/include/common/mir/input/mir_keyboard_config.h
  ${PROJECT_SOURCE_DIR}/include/common/mir/input/keyboard_leds.h
  ${MIR_COMMON_SOURCES}
)

target_include_directories(mircommon
  PUBLIC
    ${PROJECT_SOURCE_DIR}/include/common
)

target_link_libraries(mircommon
  PUBLIC
    mircore
    ${MIR_COMMON_REFERENCES}
    PkgConfig::XKBCOMMON
)


set_target_properties(mircommon

  PROPERTIES
  SOVERSION ${MIRCOMMON_ABI}
  LINK_FLAGS "-Wl,--exclude-libs=ALL -Wl,--version-script,${symbol_map}"
  LINK_DEPENDS ${symbol_map}
)

install(TARGETS mircommon LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

install(
  DIRECTORY ${CMAKE_SOURCE_DIR}/include/common/mir
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/mircommon"
)

install(
  DIRECTORY ${CMAKE_SOURCE_DIR}/src/include/common/mir
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/mircommon-internal"
)

install(
  DIRECTORY ${CMAKE_SOURCE_DIR}/include/common/mir_toolkit
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/mircommon"
)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/mircommon.pc.in
  ${CMAKE_CURRENT_BINARY_DIR}/mircommon.pc
  @ONLY
)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/mircommon-internal.pc.in
  ${CMAKE_CURRENT_BINARY_DIR}/mircommon-internal.pc
  @ONLY
)

install(
  FILES ${CMAKE_CURRENT_BINARY_DIR}/mircommon.pc
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)
install(
  FILES ${CMAKE_CURRENT_BINARY_DIR}/mircommon-internal.pc
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)

add_custom_target(
    generate-mircommon-symbols-map
    COMMAND ${PROJECT_SOURCE_DIR}/tools/symbols_map_generator/main.py
        --library-name mir_common
        --version ${MIR_VERSION_MAJOR}.${MIR_VERSION_MINOR}
        --symbols-map-path="${CMAKE_CURRENT_SOURCE_DIR}/symbols.map"
        --external-headers-directory="${PROJECT_SOURCE_DIR}/include/common"
        --internal-headers-directory="${PROJECT_SOURCE_DIR}/src/include/common"
        --include-dirs="$<JOIN:$<TARGET_PROPERTY:mirserver,INCLUDE_DIRECTORIES>,:>"
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")

add_custom_target(
    check-mircommon-symbols-map
    COMMAND ${PROJECT_SOURCE_DIR}/tools/symbols_map_generator/main.py
        --library-name mir_common
        --version ${MIR_VERSION_MAJOR}.${MIR_VERSION_MINOR}
        --symbols-map-path="${CMAKE_CURRENT_SOURCE_DIR}/symbols.map"
        --external-headers-directory="${PROJECT_SOURCE_DIR}/include/common"
        --internal-headers-directory="${PROJECT_SOURCE_DIR}/src/include/common"
        --include-dirs="$<JOIN:$<TARGET_PROPERTY:mirserver,INCLUDE_DIRECTORIES>,:>"
        --diff
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")
