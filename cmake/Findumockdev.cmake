if (umockdev_INCLUDE_DIR)
  # Already in cache, be silent
  set(umockdev_FIND_QUIETLY TRUE)
endif ()

find_path(umockdev_INCLUDE_DIR umockdev-1.0/umockdev.h)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(umockdev DEFAULT_MSG umockdev_INCLUDE_DIR)

mark_as_advanced(umockdev_INCLUDE_DIR)
