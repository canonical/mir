/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
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

mtf::PlacementApplyingShell::PlacementApplyingShell(
    std::shared_ptr<mir::shell::Shell> wrapped_coordinator,
    ClientInputRegions const& client_input_regions,
    ClientPositions const& client_positions) :
    mir::shell::ShellWrapper(wrapped_coordinator),
    client_input_regions(client_input_regions),
    client_positions(client_positions)
{
}

mtf::PlacementApplyingShell::~PlacementApplyingShell() = default;

auto mtf::PlacementApplyingShell::create_surface(
    std::shared_ptr<mir::scene::Session> const& session,
    mir::scene::SurfaceCreationParameters const& params,
    std::shared_ptr<mir::scene::SurfaceObserver> const& observer) -> std::shared_ptr<mir::scene::Surface>
{
    auto creation_parameters = params;

    auto const surface = wrapped->create_surface(session, creation_parameters, observer);
    latest_surface = surface;

    auto position= client_positions.find(params.name);
    if (position != client_positions.end())
    {
        surface->move_to(position->second.top_left);
        surface->resize(position->second.size);
    }

    auto regions = client_input_regions.find(params.name);
    if (regions != client_input_regions.end())
        surface->set_input_region(regions->second);

    return surface;
}
    
void mtf::PlacementApplyingShell::modify_surface(
    std::shared_ptr<mir::scene::Session> const& session,
    std::shared_ptr<mir::scene::Surface> const& surface,
    mir::shell::SurfaceSpecification const& modifications)
{
    mir::shell::ShellWrapper::modify_surface(session, surface, modifications);

    std::unique_lock<decltype(mutex)> lk(mutex);
    modified = true;
    cv.notify_all(); 
}

bool mtf::PlacementApplyingShell::wait_for_modify_surface(std::chrono::seconds timeout)
{
    std::unique_lock<decltype(mutex)> lk(mutex);
    return cv.wait_for(lk, timeout, [this] { return modified; }); 
}
