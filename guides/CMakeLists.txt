# Copyright © 2012 Canonical Ltd.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 3 as
# published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

PROJECT(guides)

ADD_CUSTOM_COMMAND(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/cppguide.html
    COMMAND xsltproc -o ${CMAKE_CURRENT_BINARY_DIR}/cppguide.html ${CMAKE_CURRENT_SOURCE_DIR}/styleguide.xsl ${CMAKE_CURRENT_SOURCE_DIR}/cppguide.xml
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/styleguide.xsl ${CMAKE_CURRENT_SOURCE_DIR}/cppguide.xml
    )

ADD_CUSTOM_COMMAND(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/styleguide.css
    COMMAND cp ${CMAKE_CURRENT_SOURCE_DIR}/styleguide.css ${CMAKE_CURRENT_BINARY_DIR}/styleguide.css
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/styleguide.css
    )

ADD_CUSTOM_TARGET(guides ALL
  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/cppguide.html
          ${CMAKE_CURRENT_BINARY_DIR}/styleguide.css)
