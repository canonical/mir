add_subdirectory(example-server-lib)

add_subdirectory(miral-shell)
add_subdirectory(miral-kiosk)
add_subdirectory(miral-system-compositor)
add_subdirectory(mir_demo_server)
add_subdirectory(mir-x11-kiosk)

add_subdirectory(client)

if(MIR_ENABLE_TESTS)
    add_subdirectory(tests)
endif()
