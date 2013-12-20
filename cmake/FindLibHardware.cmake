# Variables defined by this module:
message(${LIBHARDWARE_LIBRARY})
#   LIBHARDWARE_FOUND
#   LIBHARDWARE_LIBRARIES

INCLUDE(FindPackageHandleStandardArgs)

set(ENV{PKG_CONFIG_PATH} ${MIR_NDK_PATH}/usr/lib/pkgconfig)
find_package( PkgConfig )

pkg_check_modules(ANDROID_HEADERS REQUIRED android-headers)

include_directories(SYSTEM ${MIR_NDK_PATH}${ANDROID_HEADERS_INCLUDE_DIRS} PARENT_SCOPE)

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
