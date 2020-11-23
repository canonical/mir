set(MIROIL_ABI 1)
set(symbol_map ${CMAKE_CURRENT_SOURCE_DIR}/symbols.map)
set(miroil_include ${PROJECT_SOURCE_DIR}/include/miroil)

add_definitions(-DMIR_LOG_COMPONENT_FALLBACK="miroil" -DMIROIL_ENABLE_DEPRECATIONS=0)

#add_library(miroil-internal STATIC
#)
#
#set_target_properties(miroil-internal
#    PROPERTIES COMPILE_FLAGS "${CMAKE_CXXFLAGS}  -fvisibility=hidden")
#
#target_include_directories(miroil-internal
#    PRIVATE "${miroil_include}" ${MIRCLIENT_INCLUDE_DIRS}
#    PRIVATE ${MIRSERVER_INCLUDE_DIRS} ${GIO_INCLUDE_DIRS}
#    ${PROJECT_SOURCE_DIR}/include/platform
#    ${PROJECT_SOURCE_DIR}/include/client
#    ${PROJECT_SOURCE_DIR}/src/include/server
#)
#
#target_link_libraries(miroil-internal
#    PRIVATE
#        mirserver
#)

add_library(miroil SHARED
    edid.cpp ${miroil_include}/edid.h
    mirbuffer.cpp ${miroil_include}/mirbuffer.h
    persist_display_config.cpp ${miroil_include}/persist_display_config.h
)

target_include_directories(miroil
    PUBLIC  "${miroil_include}" "${PROJECT_SOURCE_DIR}/include/miral"
    PRIVATE ${MIRSERVER_INCLUDE_DIRS}  ${MIRCLIENT_INCLUDE_DIRS}
    ${PROJECT_SOURCE_DIR}/include/platform
    ${PROJECT_SOURCE_DIR}/include/renderers/gl
    ${PROJECT_SOURCE_DIR}/include/server
    ${PROJECT_SOURCE_DIR}/include/client
    ${PROJECT_SOURCE_DIR}/src/include/server
)

target_link_libraries(miroil
    PUBLIC
        mircommon
    PRIVATE
#        miroil-internal
        mirserver
        mirclient
)

set_target_properties(miroil
        PROPERTIES
        SOVERSION ${MIROIL_ABI}
        LINK_FLAGS "-Wl,--exclude-libs=ALL -Wl,--version-script,${symbol_map}"
        LINK_DEPENDS ${symbol_map}
)

if (CMAKE_COMPILER_IS_GNUCXX   AND  # clang generates slightly different symbols (but we don't care)
    MIR_LINK_TIME_OPTIMIZATION AND  # g++ generates slightly different symbols without LTO (but we don't care)
    EXISTS /etc/debian_version)     # Using dpkg-gensymbols only makes sense on Debian based distros

    find_program(MIR_DPKG_GENSYMBOLS dpkg-gensymbols)
    if (MIR_DPKG_GENSYMBOLS)
        add_custom_target(check-miroil-symbols ALL
            DEPENDS miroil ${PROJECT_SOURCE_DIR}/debian/libmiroil${MIROIL_ABI}.symbols
            COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/check-and-update-debian-symbols.py ${CMAKE_LIBRARY_OUTPUT_DIRECTORY} miroil ${MIROIL_VERSION} ${MIROIL_ABI}
            WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
            VERBATIM)
    endif()
endif()

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/miroil.pc.in
    ${CMAKE_CURRENT_BINARY_DIR}/miroil.pc
    @ONLY
)

if(TARGET doc)
    add_custom_target(regenerate-miroil-symbols-map
            ${CMAKE_CURRENT_SOURCE_DIR}/regenerate-miroil-symbols-map.py ${CMAKE_BINARY_DIR}/doc/xml/*.xml > ${symbol_map}
            DEPENDS doc)
endif()

install(TARGETS     miroil                           LIBRARY DESTINATION "${CMAKE_INSTALL_FULL_LIBDIR}")
install(DIRECTORY   ${CMAKE_SOURCE_DIR}/include/miroil       DESTINATION "${CMAKE_INSTALL_PREFIX}/include")
install(FILES       ${CMAKE_CURRENT_BINARY_DIR}/miroil.pc    DESTINATION "${CMAKE_INSTALL_FULL_LIBDIR}/pkgconfig")
