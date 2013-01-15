# Variables defined by this module:
#   LIBHARDWARE_FOUND
#   LIBHARDWARE_INCLUDE_DIRS
#   LIBHARDWARE_LIBRARIES

INCLUDE(FindPackageHandleStandardArgs)

set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE BOTH)
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
   )

#todo: the function we need from libui.so is compiled into two different places, depending on whether we're 
#      using bionic or not
if (MIR_USES_BIONIC)
# if we're using bionic, then libui.so has all the symbols we need
  find_library(ANDROID_UI_LIBRARY
    NAMES         libui.so
                  libutils.so
                  libpixelflinger.so
                  libhardware_legacy.so
                  libskia.so
                  libbinder.so
                  libwpa_client.so
                  libnetutils.so
                  libemoji
                  libjpeg 
                  libexpat
    HINTS         ${ANDROID_STANDALONE_TOOLCHAIN}/sysroot/usr/lib
                  ${ANDROID_STANDALONE_TOOLCHAIN}/sysroot/lib
   )

else()
# if we're using a hybris/chroot compile, then libui functions are linked via hybris.
# hybris currently (confusingly) compiles the symbols we need for libui.so into libEGL.so (this should be
# corrected in the hybris project)
  if (EGL_FOUND)
    set(ANDROID_UI_LIBRARY ${EGL_LIBRARIES})
  else()
    set(ANDROID_UI_LIBRARY)
    set(LIBHARDWARE_FOUND false)
  endif() 
endif()

set(ANDROID_UI_LIBRARIES ${ANDROID_UI_LIBRARY})
set(ANDROID_UI_INCLUDE_DIRS ${ANDROID_UI_INCLUDE_DIR})

# handle the QUIETLY and REQUIRED arguments and set ANDROID_UI_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(ANDROID_UI DEFAULT_MSG
                                  ANDROID_UI_LIBRARY ANDROID_UI_INCLUDE_DIR)

mark_as_advanced(ANDROID_UI_INCLUDE_DIR ANDROID_UI_LIBRARY )


