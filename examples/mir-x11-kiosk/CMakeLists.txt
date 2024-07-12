mir_add_wrapped_executable(mir-x11-kiosk
    x11_kiosk_main.cpp
    x11_kiosk_window_manager.cpp x11_kiosk_window_manager.h
)

target_link_libraries(mir-x11-kiosk PRIVATE miral)

add_custom_target(mir-x11-kiosk-launch ALL
    cp ${CMAKE_CURRENT_SOURCE_DIR}/mir-x11-kiosk-launch.sh ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/mir-x11-kiosk-launch
)

install(
    PROGRAMS
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/mir-x11-kiosk
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/mir-x11-kiosk-launch
    DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
)
