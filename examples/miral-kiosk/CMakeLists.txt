include_directories(
    ${PROJECT_SOURCE_DIR}/include/miral
    ${PROJECT_SOURCE_DIR}/include/client
)

mir_add_wrapped_executable(miral-kiosk
    kiosk_main.cpp
    kiosk_window_manager.cpp    kiosk_window_manager.h
    sw_splash.cpp               sw_splash.h
)

target_link_libraries(miral-kiosk miral)
