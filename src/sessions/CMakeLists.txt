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
#
# Authored by: Alan Griffiths <alan@octopull.co.uk>


set(
  SESSIONS_SOURCES

  application_session.cpp
  session_container.cpp
  session_manager.cpp
  registration_order_focus_sequence.cpp
  single_visibility_focus_mechanism.cpp
  consuming_placement_strategy.cpp
  organising_surface_factory.cpp
  surface_creation_parameters.cpp
)

add_library(
  mirsessions STATIC

  ${SESSIONS_SOURCES}
)
