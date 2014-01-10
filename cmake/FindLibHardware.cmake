# Variables defined by this module:
#message(${LIBHARDWARE_LIBRARY})
#   LIBHARDWARE_FOUND
#   LIBHARDWARE_LIBRARIES
#   LIBHARDWARE_INCLUDE_DIRS

INCLUDE(FindPackageHandleStandardArgs)

find_package( PkgConfig )
pkg_check_modules(ANDROID_HEADERS REQUIRED android-headers)
set(LIBHARDWARE_INCLUDE_DIRS ${ANDROID_HEADERS_INCLUDE_DIRS})

find_library(LIBHARDWARE_LIBRARY
   NAMES         libhardware.so.2
                 libhardware.so 
)

set(LIBHARDWARE_LIBRARIES ${LIBHARDWARE_LIBRARY})

# handle the QUIETLY and REQUIRED arguments and set LIBHARDWARE_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(LIBHARDWARE DEFAULT_MSG
                                  LIBHARDWARE_LIBRARY)

mark_as_advanced(LIBHARDWARE_INCLUDE_DIR LIBHARDWARE_LIBRARY )
