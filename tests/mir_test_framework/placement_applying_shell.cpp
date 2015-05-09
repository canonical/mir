/*
 * Copyright © 2015 Canonical Ltd.
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
 * Authored by: Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#include "mir_test_framework/placement_applying_shell.h"

namespace mtf = mir_test_framework;

mtf::PlacementApplyingShell::PlacementApplyingShell(std::shared_ptr<mir::shell::Shell> wrapped_coordinator,
                                                    ClientInputRegions const& client_input_regions,
                                                    ClientPositions const& client_positions,
                                                    ClientDepths const& client_depths) :
    mir::shell::ShellWrapper(wrapped_coordinator),
    client_input_regions(client_input_regions),
    client_positions(client_positions),
    client_depths(client_depths)
{
}

mir::frontend::SurfaceId mtf::PlacementApplyingShell::create_surface(
        std::shared_ptr<mir::scene::Session> const& session,
        mir::scene::SurfaceCreationParameters const& params)
{
    auto creation_parameters = params;

    auto depth = client_depths.find(params.name);
    if (depth != client_depths.end())
        creation_parameters.depth = depth->second;

    auto const id = wrapped->create_surface(session, creation_parameters);
    auto const surface = session->surface(id);

    auto position= client_positions.find(params.name);
    if (position != client_positions.end())
    {
        surface->move_to(position->second.top_left);
        surface->resize(position->second.size);
    }

    auto regions = client_input_regions.find(params.name);
    if (regions != client_input_regions.end())
        surface->set_input_region(regions->second);

    return id;
}
