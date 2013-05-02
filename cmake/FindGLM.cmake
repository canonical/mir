# - Try to find GLM
# Once done this will define
#  GLM_FOUND - System has GLM
#  GLM_INCLUDE_DIRS - The GLM include directories

find_path(GLM_INCLUDE_DIR glm/glm.hpp)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GLM DEFAULT_MSG GLM_INCLUDE_DIR)

mark_as_advanced(GLM_INCLUDE_DIR)
