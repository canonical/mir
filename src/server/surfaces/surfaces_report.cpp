/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "mir/surfaces/surfaces_report.h"

namespace ms = mir::surfaces;

void ms::NullSurfacesReport::surface_created(Surface* const /*surface*/) {}
void ms::NullSurfacesReport::surface_added(Surface* const /*surface*/) {}
void ms::NullSurfacesReport::surface_removed(Surface* const /*surface*/) {}
void ms::NullSurfacesReport::surface_deleted(Surface* const /*surface*/) {}

