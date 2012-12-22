# Variables defined by this module:
#   LIBHARDWARE_FOUND
#   LIBHARDWARE_INCLUDE_DIRS
#   LIBHARDWARE_LIBRARIES

INCLUDE(FindPackageHandleStandardArgs)

find_path(ANDROID_UI_INCLUDE_DIR
   NAMES         ui/FramebufferNativeWindow.h
                 pixelflinger/pixelflinger.h
                 pixelflinger/format.h
                 cutils/atomic.h
                 utils/threads.h
                 utils/String8.h
                 utils/Errors.h
                 utils/RefBase.h
                 utils/Timers.h
                 utils/SharedBuffer.h
                 utils/Unicode.h
                 utils/TypeHelpers.h
                 utils/StrongPointer.h
                 ui/Rect.h
                 ui/Point.h
                 ui/egl/android_natives.h
   HINTS         /opt/bundle/junk
   )

#find_library(ANDROID_UI_LIBRARY
#   NAMES         libui.so
#                 libutils.so
#                 libpixelflinger.so
#                 libhardware_legacy.so
#                 libskia.so
#                 libbinder.so
#                 libwpa_client.so
#                 libnetutils.so
#                 libemoji
#                 libjpeg 
#                 libexpat
#   HINTS         ${ANDROID_STANDALONE_TOOLCHAIN}/sysroot/usr/lib
#                 ${ANDROID_STANDALONE_TOOLCHAIN}/sysroot/lib
#   )

#set(ANDROID_UI_LIBRARIES ${ANDROID_UI_LIBRARY})
set(ANDROID_UI_INCLUDE_DIRS ${ANDROID_UI_INCLUDE_DIR})

# handle the QUIETLY and REQUIRED arguments and set ANDROID_UI_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(ANDROID_UI DEFAULT_MSG
                                  ANDROID_UI_INCLUDE_DIR)
                                  #ANDROID_UI_LIBRARY ANDROID_UI_INCLUDE_DIR)

mark_as_advanced(ANDROID_UI_INCLUDE_DIR ANDROID_UI_LIBRARY )


