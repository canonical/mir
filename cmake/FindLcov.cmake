# - Find lcov 
# Will define:
#
# LCOV_EXECUTABLE - the lcov binary
# GENHTML_EXECUTABLE - the genhtml executable
#
# Copyright (C) 2010 by Johannes Wienke <jwienke at techfak dot uni-bielefeld dot de>
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

FIND_PROGRAM(LCOV_EXECUTABLE lcov)
FIND_PROGRAM(GENHTML_EXECUTABLE genhtml)

FIND_PACKAGE_HANDLE_STANDARD_ARGS(Lcov DEFAULT_MSG LCOV_EXECUTABLE GENHTML_EXECUTABLE)

# only visible in advanced view
MARK_AS_ADVANCED(LCOV_EXECUTABLE GENHTML_EXECUTABLE)
