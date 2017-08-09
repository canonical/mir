# Copyright © 2013 Canonical Ltd.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License version 2 or 3 as
# published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
# Authored by: Daniel van Vugt <daniel.van.vugt@canonical.com>

add_library(mirsharedenv OBJECT
  default_configuration.cpp
)

list(APPEND MIR_COMMON_SOURCES
  $<TARGET_OBJECTS:mirsharedenv>
)

set(MIR_COMMON_SOURCES ${MIR_COMMON_SOURCES} PARENT_SCOPE)
