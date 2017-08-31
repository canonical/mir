if(${CMAKE_COMPILER_IS_GNUCXX})
    set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} -flto")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -flto")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -flto")
    set(CMAKE_AR "gcc-ar")
    set(CMAKE_NM "gcc-nm")
    set(CMAKE_RANLIB "gcc-ranlib")
endif()

include_directories(
  ${PROJECT_SOURCE_DIR}/include/platform
  ${PROJECT_SOURCE_DIR}/include/client
  ${PROJECT_SOURCE_DIR}/include/server
)

set(MIRAL_ABI 1)
set(symbol_map ${CMAKE_CURRENT_SOURCE_DIR}/symbols.map)
set(miral_include ${PROJECT_SOURCE_DIR}/include/miral)

add_library(mirclientcpp INTERFACE)

add_library(miral-internal STATIC
    basic_window_manager.cpp            basic_window_manager.h window_manager_tools_implementation.h
    coordinate_translator.cpp           coordinate_translator.h
    display_configuration_listeners.cpp display_configuration_listeners.h
    mru_window_list.cpp                 mru_window_list.h
    window_management_trace.cpp         window_management_trace.h
    xcursor_loader.cpp                  xcursor_loader.h
    xcursor.c                           xcursor.h
                                        both_versions.h
                                        join_client_threads.h
)

set_source_files_properties(xcursor.c PROPERTIES COMPILE_DEFINITIONS _GNU_SOURCE)

add_library(mirserveral SHARED
    active_outputs.cpp                  ${miral_include}/miral/active_outputs.h
    add_init_callback.cpp               ${miral_include}/miral/add_init_callback.h
    application.cpp                     ${miral_include}/miral/application.h
    application_authorizer.cpp          ${miral_include}/miral/application_authorizer.h
    application_info.cpp                ${miral_include}/miral/application_info.h
    canonical_window_manager.cpp        ${miral_include}/miral/canonical_window_manager.h
    command_line_option.cpp             ${miral_include}/miral/command_line_option.h
    cursor_theme.cpp                    ${miral_include}/miral/cursor_theme.h
    debug_extension.cpp                 ${miral_include}/miral/debug_extension.h
    keymap.cpp                          ${miral_include}/miral/keymap.h
    runner.cpp                          ${miral_include}/miral/runner.h
    display_configuration_option.cpp    ${miral_include}/miral/display_configuration_option.h
    output.cpp                          ${miral_include}/miral/output.h
    append_event_filter.cpp             ${miral_include}/miral/append_event_filter.h
    window.cpp                          ${miral_include}/miral/window.h
    window_info.cpp                     ${miral_include}/miral/window_info.h
    window_management_options.cpp       ${miral_include}/miral/window_management_options.h
    window_specification.cpp            ${miral_include}/miral/window_specification.h
    internal_client.cpp                 ${miral_include}/miral/internal_client.h
    set_command_line_handler.cpp        ${miral_include}/miral/set_command_line_handler.h
    set_terminator.cpp                  ${miral_include}/miral/set_terminator.h
    set_window_management_policy.cpp    ${miral_include}/miral/set_window_management_policy.h
    workspace_policy.cpp                ${miral_include}/miral/workspace_policy.h
    window_management_policy.cpp        ${miral_include}/miral/window_management_policy.h
    window_manager_tools.cpp            ${miral_include}/miral/window_manager_tools.h
                                        ${miral_include}/miral/window_management_policy_addendum2.h
                                        ${miral_include}/miral/window_management_policy_addendum3.h
)

target_include_directories(mirclientcpp
    INTERFACE "${miral_include}" ${MIRCLIENT_INCLUDE_DIRS}
)

target_include_directories(miral-internal
    PRIVATE "${miral_include}" ${MIRCLIENT_INCLUDE_DIRS}
    PRIVATE ${MIRSERVER_INCLUDE_DIRS}
)

target_include_directories(mirserveral
    PUBLIC  "${miral_include}" ${MIRCLIENT_INCLUDE_DIRS}
    PRIVATE ${MIRSERVER_INCLUDE_DIRS}
)

target_link_libraries(mirserveral
    PUBLIC
        mirclient
        mircommon
    PRIVATE
        miral-internal
        mirserver
)

set_target_properties(mirserveral
        PROPERTIES
        SOVERSION ${MIRAL_ABI}
        LINK_FLAGS "-Wl,--exclude-libs=ALL -Wl,--version-script,${symbol_map}"
        LINK_DEPENDS ${symbol_map}
)

# clang generates slightly different symbols (but we don't care)
if (CMAKE_COMPILER_IS_GNUCXX)
    add_custom_target(check-miral-symbols ALL
            DEPENDS mirserveral ${PROJECT_SOURCE_DIR}/debian/libmirserveral${MIRAL_ABI}.symbols
            COMMAND rm -f ${CMAKE_CURRENT_BINARY_DIR}/libmirserveral${MIRAL_ABI}.symbols
            COMMAND dpkg-gensymbols -e${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libmirserveral.so.${MIRAL_ABI} -plibmirserveral${MIRAL_ABI} -O${CMAKE_CURRENT_BINARY_DIR}/libmirserveral${MIRAL_ABI}.symbols
            WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
            VERBATIM
    )
endif()

set(LIBDIR "${CMAKE_INSTALL_FULL_LIBDIR}")
set(INCLUDEDIR "${CMAKE_INSTALL_PREFIX}/include")

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/miral.pc.in
    ${CMAKE_CURRENT_BINARY_DIR}/miral.pc
    @ONLY
)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/version.h.in
    ${miral_include}/miral/version.h
)

install(TARGETS     mirserveral LIBRARY                     DESTINATION "${CMAKE_INSTALL_FULL_LIBDIR}")
install(DIRECTORY   ${CMAKE_SOURCE_DIR}/include/miral       DESTINATION "${INCLUDEDIR}")
install(FILES       ${CMAKE_CURRENT_BINARY_DIR}/miral.pc    DESTINATION "${CMAKE_INSTALL_FULL_LIBDIR}/pkgconfig")
