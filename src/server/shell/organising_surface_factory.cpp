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
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#include "organising_surface_factory.h"
#include "mir/shell/surface_creation_parameters.h"
#include "mir/scene/surface_coordinator.h"
#include "mir/scene/surface.h"

#include <cstdlib>

namespace mf = mir::frontend;
namespace ms = mir::scene;
namespace msh = mir::shell;

msh::OrganisingSurfaceFactory::OrganisingSurfaceFactory(
    std::shared_ptr<scene::SurfaceCoordinator> const& surface_coordinator) :
    surface_coordinator(surface_coordinator)
{
}

msh::OrganisingSurfaceFactory::~OrganisingSurfaceFactory()
{
}

std::shared_ptr<ms::Surface> msh::OrganisingSurfaceFactory::create_surface(
    Session* session,
    SurfaceCreationParameters const& params,
    std::shared_ptr<scene::SurfaceObserver> const& observer)
{
    return surface_coordinator->add_surface(params, session, observer);
}

void msh::OrganisingSurfaceFactory::destroy_surface(std::shared_ptr<ms::Surface> const& surface)
{
    surface_coordinator->remove_surface(surface);
}
