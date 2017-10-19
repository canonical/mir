include_directories(
        ${PROJECT_SOURCE_DIR}/include/miral
        ${PROJECT_SOURCE_DIR}/include/client
)

add_subdirectory(spinner)
add_subdirectory(desktop)

add_custom_target(miral-run ALL
    cp ${CMAKE_CURRENT_SOURCE_DIR}/miral-run.sh ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/miral-run
)

install(PROGRAMS ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/miral-run
    DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
)

add_custom_target(miral-xrun ALL
    cp ${CMAKE_CURRENT_SOURCE_DIR}/miral-xrun.sh ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/miral-xrun
)

install(PROGRAMS ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/miral-xrun
    DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
)

add_custom_target(miral-screencast ALL
    cp ${CMAKE_CURRENT_SOURCE_DIR}/miral-screencast.sh ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/miral-screencast
)

install(PROGRAMS ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/miral-screencast
    DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
)

add_custom_target(miral-desktop ALL
    cp ${CMAKE_CURRENT_SOURCE_DIR}/miral-desktop.sh ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/miral-desktop
)

install(PROGRAMS ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/miral-desktop
    DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
)

add_custom_target(miral-app ALL
    cp ${CMAKE_CURRENT_SOURCE_DIR}/miral-app.sh ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/miral-app
)

install(PROGRAMS ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/miral-app
    DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
)

add_library(miral-shell-lib STATIC
    tiling_window_manager.cpp   tiling_window_manager.h
    floating_window_manager.cpp floating_window_manager.h
    decoration_provider.cpp     decoration_provider.h
    titlebar_config.cpp         titlebar_config.h
)

if (EXISTS /usr/share/fonts/truetype/ubuntu-font-family/Ubuntu-B.ttf)
 set_source_files_properties(titlebar_config.cpp PROPERTIES
   COMPILE_DEFINITIONS MIRAL_DEFAULT_FONT_FILE="/usr/share/fonts/truetype/ubuntu-font-family/Ubuntu-B.ttf")
else()
 execute_process(COMMAND find  /usr/share/fonts/ -name FreeSansBold.ttf
   OUTPUT_VARIABLE MIR_DEFAULT_FONT_FILE OUTPUT_STRIP_TRAILING_WHITESPACE)
 set_source_files_properties(titlebar_config.cpp PROPERTIES
   COMPILE_DEFINITIONS MIRAL_DEFAULT_FONT_FILE="${MIR_DEFAULT_FONT_FILE}")
endif()

pkg_check_modules(FREETYPE freetype2 REQUIRED)
target_include_directories(miral-shell-lib PRIVATE ${FREETYPE_INCLUDE_DIRS})
target_compile_definitions(miral-shell-lib PRIVATE -DTYPO_SUPPORTS_FREETYPE)
target_link_libraries(miral-shell-lib miral-spinner miral ${FREETYPE_LIBRARIES})

mir_add_wrapped_executable(miral-shell
    shell_main.cpp
)

target_link_libraries(miral-shell
    miral-shell-lib
    miral
)

