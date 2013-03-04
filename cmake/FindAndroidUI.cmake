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

                 #we keep these headers in-tree, so search in-tree
                 NO_CMAKE_FIND_ROOT_PATH
   )

# we are using a hybris/chroot compile, so libui functions are linked via hybris.
# hybris currently (confusingly) compiles the symbols we need for libui.so into libEGL.so (this should be corrected in the hybris project)
if (EGL_FOUND)
  set(ANDROID_UI_LIBRARY ${EGL_LIBRARIES})
else()
  set(ANDROID_UI_LIBRARY)
  set(LIBHARDWARE_FOUND false)
endif() 

set(ANDROID_UI_LIBRARIES ${ANDROID_UI_LIBRARY})
set(ANDROID_UI_INCLUDE_DIRS ${ANDROID_UI_INCLUDE_DIR})

# handle the QUIETLY and REQUIRED arguments and set ANDROID_UI_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(ANDROID_UI DEFAULT_MSG
                                  ANDROID_UI_LIBRARY ANDROID_UI_INCLUDE_DIR)

mark_as_advanced(ANDROID_UI_INCLUDE_DIR ANDROID_UI_LIBRARY )
