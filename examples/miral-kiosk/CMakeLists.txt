mir_add_wrapped_executable(miral-kiosk
    kiosk_main.cpp
    kiosk_window_manager.cpp    kiosk_window_manager.h
)

target_link_libraries(miral-kiosk example-shell-lib)

# I'm not sure why this is needed in a chroot, but it's not worth finding out
if(DEFINED MIR_CHROOT)
    target_link_libraries(miral-kiosk EGL PkgConfig::GLESv2)
endif()

