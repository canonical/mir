# Variables defined by this module:
#   ANDROID_PROPERTIES_FOUND
#   ANDROID_PROPERTIES_LIBRARIES
#   ANDROID_PROPERTIES_INCLUDE_DIRS

INCLUDE(FindPackageHandleStandardArgs)

find_package( PkgConfig )
pkg_check_modules(ANDROID_PROPERTIES REQUIRED libandroid-properties)

find_path(ANDROID_PROPERTIES_INCLUDE_DIR hybris/properties/properties.h
          HINTS ${PC_ANDROID_PROPERTIES_INCLUDEDIR} ${PC_ANDROID_PROPERTIES_INCLUDE_DIRS})
find_library(ANDROID_PROPERTIES_LIBRARIES
             NAMES        libandroid-properties.so
             HINTS ${PC_ANDROID_PROPERTIES_LIBDIR} ${PC_ANDROID_PROPERTIES_LIBRARY_DIRS})

# handle the QUIETLY and REQUIRED arguments and set ANDROID_PROPERTIES_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(ANDROID_PROPERTIES DEFAULT_MSG
                                  ANDROID_PROPERTIES_LIBRARIES)

mark_as_advanced(ANDROID_PROPERTIES_INCLUDE_DIR ANDROID_PROPERTIES_LIBRARY )
