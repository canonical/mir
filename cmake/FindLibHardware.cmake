# Variables defined by this module:
#   LIBHARDWARE_FOUND
#   LIBHARDWARE_INCLUDE_DIRS
#   LIBHARDWARE_LIBRARIES

INCLUDE(FindPackageHandleStandardArgs)


#find_path(LIBHARDWARE_INCLUDE_DIR
#   NAMES         hardware/hardware.h
#                 hardware/gralloc.h
#                 hardware/hwcomposer.h
#                 cutils/native_handle.h
#                 system/graphics.h
#                 system/window.h
#   )

find_library(LIBHARDWARE_LIBRARY
   NAMES         libhardware.so.2
                 libhardware.so 
)

message(${LIBHARDWARE_LIBRARY})
set(LIBHARDWARE_LIBRARIES ${LIBHARDWARE_LIBRARY})
#set(LIBHARDWARE_INCLUDE_DIRS ${LIBHARDWARE_INCLUDE_DIR})

# handle the QUIETLY and REQUIRED arguments and set LIBHARDWARE_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(LIBHARDWARE DEFAULT_MSG
                                  LIBHARDWARE_LIBRARY)

mark_as_advanced(LIBHARDWARE_INCLUDE_DIR LIBHARDWARE_LIBRARY )
