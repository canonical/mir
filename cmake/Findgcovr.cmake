# - Find gcovr scrip 
# Will define:
#
# GCOVR_EXECUTABLE - the gcovr script
#
# Uses:
#
# GCOVR_ROOT - root to search for the script
#
# Copyright (C) 2011 by Johannes Wienke <jwienke at techfak dot uni-bielefeld dot de>
#
# This program is free software; you can redistribute it
# and/or modify it under the terms of the GNU General
# Public License as published by the Free Software Foundation;
# either version 2, or (at your option)
# any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#

INCLUDE(FindPackageHandleStandardArgs)

FIND_PROGRAM(GCOVR_EXECUTABLE gcovr HINTS ${GCOVR_ROOT} "${GCOVR_ROOT}/bin")

FIND_PACKAGE_HANDLE_STANDARD_ARGS(gcovr DEFAULT_MSG GCOVR_EXECUTABLE)

# only visible in advanced view
MARK_AS_ADVANCED(GCOVR_EXECUTABLE)
