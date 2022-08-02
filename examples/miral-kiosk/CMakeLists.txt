mir_add_wrapped_executable(miral-kiosk
    kiosk_main.cpp
    kiosk_window_manager.cpp    kiosk_window_manager.h
)

target_link_libraries(miral-kiosk example-shell-lib)

# I'm not sure why this is needed in a chroot, but it's not worth finding out
if(DEFINED MIR_CHROOT)
    target_link_libraries(miral-kiosk EGL ${GLESv2_LDFLAGS})
endif()

add_custom_target(fake-mir-kiosk ALL
    cp ${CMAKE_CURRENT_SOURCE_DIR}/fake-mir-kiosk.sh ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/fake-mir-kiosk
)

install(PROGRAMS ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/fake-mir-kiosk
    DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
)
