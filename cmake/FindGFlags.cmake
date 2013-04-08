if (GFlags_INCLUDE_DIR)
  # Already in cache, be silent
  set(GFlags_FIND_QUIETLY TRUE)
endif ()

find_path(GFlags_INCLUDE_DIR gflags/gflags.h)

find_library(GFlags_LIBRARY libgflags.so
             HINTS /usr/lib/arm-linux-gnueabihf/)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GFlags DEFAULT_MSG GFlags_LIBRARY GFlags_INCLUDE_DIR)

mark_as_advanced(GFlags_LIBRARY GFlags_INCLUDE_DIR)
