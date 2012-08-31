# Variables defined by this module:
#   LIBHARDWARE_FOUND
#   LIBHARDWARE_INCLUDE_DIRS
#   LIBHARDWARE_LIBRARIES

INCLUDE(FindPackageHandleStandardArgs)

find_path(ANDROID_UI_INCLUDE_DIR
   NAMES         ui/FramebufferNativeWindow.h
                 pixelflinger/pixelflinger.h
                 ui/Rect.h
                 util/threads.h
                 util/String8.h
                 ui/egl/android_natives.h
   HINTS         ${ANDROID_STANDALONE_TOOLCHAIN}/sysroot
   )

find_library(ANDROID_UI_LIBRARY
   NAMES         libui.so
                 libutil.so 
   HINTS         ${ANDROID_STANDALONE_TOOLCHAIN}/sysroot/usr/lib
                 ${ANDROID_STANDALONE_TOOLCHAIN}/sysroot/lib
   )


# handle the QUIETLY and REQUIRED arguments and set LIBHARDWARE_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(ANDROID_UI DEFAULT_MSG
                                  ANDROID_UI_LIBRARY ANDROID_UI_INCLUDE_DIR)

mark_as_advanced(ANDROID_UI_INCLUDE_DIR ANDROID_UI_LIBRARY )


