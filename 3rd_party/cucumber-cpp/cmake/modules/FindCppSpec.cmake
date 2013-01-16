# Locate CppSpec: Behaviour driven development with C++
#
# Defines the following variables:
#
#   CPPSPEC_FOUND - Found CppSpec
#   CPPSPEC_INCLUDE_DIR - Include directories
#   CPPSPEC_LIBRARY - libCppSpec
#

find_path(CPPSPEC_INCLUDE_DIR CppSpec/CppSpec.h
    HINTS
        $ENV{CPPSPEC_ROOT}/include
        ${CPPSPEC_ROOT}/include
)

find_library(CPPSPEC_LIBRARY CppSpec
    HINTS
        $ENV{CPPSPEC_ROOT}/lib
        ${CPPSPEC_ROOT}/lib)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(CPPSPEC DEFAULT_MSG CPPSPEC_LIBRARY CPPSPEC_INCLUDE_DIR)
mark_as_advanced(CPPSPEC_LIBRARY CPPSPEC_INCLUDE_DIR)
