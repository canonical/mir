/*
 * Copyright © 2014 Canonical Ltd.
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
 *              Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#include "scene_report.h"

namespace mrn = mir::report::null;

void mrn::SceneReport::surface_created(BasicSurfaceId /*id*/, std::string const& /*name*/)
{
}
void mrn::SceneReport::surface_added(BasicSurfaceId /*id*/, std::string const& /*name*/)
{
}
void mrn::SceneReport::surface_removed(BasicSurfaceId /*id*/, std::string const& /*name*/)
{
}
void mrn::SceneReport::surface_deleted(BasicSurfaceId /*id*/, std::string const& /*name*/)
{
}

