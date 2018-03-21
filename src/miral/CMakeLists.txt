include_directories(
  ${PROJECT_SOURCE_DIR}/include/platform
  ${PROJECT_SOURCE_DIR}/include/client
  ${PROJECT_SOURCE_DIR}/include/server
)

set(MIRAL_ABI 3)
set(symbol_map ${CMAKE_CURRENT_SOURCE_DIR}/symbols.map)
set(miral_include ${PROJECT_SOURCE_DIR}/include/miral)

add_library(mirclientcpp INTERFACE)

add_library(miral-internal STATIC
    active_outputs.cpp                  active_outputs.h
    basic_window_manager.cpp            basic_window_manager.h window_manager_tools_implementation.h
    coordinate_translator.cpp           coordinate_translator.h
    display_configuration_listeners.cpp display_configuration_listeners.h
    mru_window_list.cpp                 mru_window_list.h
    window_management_trace.cpp         window_management_trace.h
    xcursor_loader.cpp                  xcursor_loader.h
    xcursor.c                           xcursor.h
                                        join_client_threads.h
                                        window_info_defaults.h
)

set_source_files_properties(xcursor.c PROPERTIES COMPILE_DEFINITIONS _GNU_SOURCE)

add_library(miral SHARED
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
    window_management_policy.cpp        ${miral_include}/miral/window_management_policy.h
    window_manager_tools.cpp            ${miral_include}/miral/window_manager_tools.h
                                        ${miral_include}/miral/lambda_as_function.h
)

check_cxx_compiler_flag(-Wno-attribute-alias HAS_W_NO_ATTRIBUTE_ALIAS)
if(HAS_W_NO_ATTRIBUTE_ALIAS)
    set_source_files_properties(
            canonical_window_manager.cpp
            internal_client.cpp
            set_window_management_policy.cpp
            window_info.cpp
            window_management_policy.cpp
            window_manager_tools.cpp
            PROPERTIES COMPILE_FLAGS "${CMAKE_CXXFLAGS} -Wno-attribute-alias")
endif()

target_include_directories(mirclientcpp
    INTERFACE "${miral_include}" ${MIRCLIENT_INCLUDE_DIRS}
)

target_include_directories(miral-internal
    PRIVATE "${miral_include}" ${MIRCLIENT_INCLUDE_DIRS}
    PRIVATE ${MIRSERVER_INCLUDE_DIRS}
)

target_include_directories(miral
    PUBLIC  "${miral_include}" ${MIRCLIENT_INCLUDE_DIRS}
    PRIVATE ${MIRSERVER_INCLUDE_DIRS}
)

target_link_libraries(miral
    PUBLIC
        mirclient
        mircommon
    PRIVATE
        miral-internal
        mirserver
)

set_target_properties(miral
        PROPERTIES
        SOVERSION ${MIRAL_ABI}
        LINK_FLAGS "-Wl,--exclude-libs=ALL -Wl,--version-script,${symbol_map}"
        LINK_DEPENDS ${symbol_map}
)



if (CMAKE_COMPILER_IS_GNUCXX   AND  # clang generates slightly different symbols (but we don't care)
    MIR_LINK_TIME_OPTIMIZATION AND  # g++ generates slightly different symbols without LTO (but we don't care)
    EXISTS /etc/debian_version)     # Using dpkg-gensymbols only makes sense on Debian based distros

    find_program(MIR_DPKG_GENSYMBOLS dpkg-gensymbols)
    if (MIR_DPKG_GENSYMBOLS)
        add_custom_target(check-miral-symbols ALL
            DEPENDS miral ${PROJECT_SOURCE_DIR}/debian/libmiral${MIRAL_ABI}.symbols
            COMMAND rm -f ${CMAKE_CURRENT_BINARY_DIR}/libmiral${MIRAL_ABI}.symbols
            COMMAND dpkg-gensymbols -e${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libmiral.so.${MIRAL_ABI} -plibmiral${MIRAL_ABI} -v${MIRAL_VERSION} -O${CMAKE_CURRENT_BINARY_DIR}/libmiral${MIRAL_ABI}.symbols
            WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
            VERBATIM)
    endif()
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

if(TARGET doc)
    add_custom_target(regenerate-miral-symbols-map
            ${CMAKE_CURRENT_SOURCE_DIR}/process_doxygen_xml.py ${CMAKE_BINARY_DIR}/doc/xml/*.xml > ${symbol_map}
            DEPENDS doc)
endif()

install(TARGETS     miral                           LIBRARY DESTINATION "${CMAKE_INSTALL_FULL_LIBDIR}")
install(DIRECTORY   ${CMAKE_SOURCE_DIR}/include/miral       DESTINATION "${INCLUDEDIR}")
install(FILES       ${CMAKE_CURRENT_BINARY_DIR}/miral.pc    DESTINATION "${CMAKE_INSTALL_FULL_LIBDIR}/pkgconfig")
