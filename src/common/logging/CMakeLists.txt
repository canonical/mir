# Copyright © Canonical Ltd.
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

add_library(mirsharedlogging OBJECT
  dumb_console_logger.cpp
  file_logger.cpp
  input_timestamp.cpp
  multi_logger.cpp
  shared_library_prober_report.cpp
  logger.cpp
)

target_include_directories(mirsharedlogging
    PUBLIC
        ${PROJECT_SOURCE_DIR}/src/include/common
)

