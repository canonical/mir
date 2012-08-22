# Variables defined by this module:
#   LibHardware_INCLUDE_DIR (not cached)
#   LibHardware_FOUND
#   LibHardware_LIBS

find_path(LibHardware_INCLUDE_DIR
   NAMES         hardware/hardware.h
                 hardware/gralloc.h
                 cutils/native_handle.h
                 system/graphics.h
                 system/window.h
   HINTS         ${ANDROID_STANDALONE_TOOLCHAIN}/sysroot
   )

find_path(LibHardware_LIBS
   NAMES         libhardware.so 
   HINTS         ${ANDROID_STANDALONE_TOOLCHAIN}/sysroot/usr/lib
                 ${ANDROID_STANDALONE_TOOLCHAIN}/sysroot/lib
   )

if (LibHardware_INCLUDE_DIR)
  if (LibHardware_LIBS)
    set(LibHardware_FOUND true)
  endif(LibHardware_LIBS)
endif(LibHardware_INCLUDE_DIR)

if(LibHardware_FOUND)
  message("libhardware found.")
else()
  if(LibHardware_FIND_REQUIRED)
    message(FATAL_ERROR "Unable to find Android libhardware libraries.")
  endif()
endif(LibHardware_FOUND)

