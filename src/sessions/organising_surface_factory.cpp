/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
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

#include "mir/sessions/organising_surface_factory.h"
#include "mir/sessions/placement_strategy.h"
#include "mir/sessions/surface_creation_parameters.h"

namespace msess = mir::sessions;

msess::OrganisingSurfaceFactory::OrganisingSurfaceFactory(std::shared_ptr<msess::SurfaceFactory> const& underlying_factory,
                                                          std::shared_ptr<msess::PlacementStrategy> const& placement_strategy)
 : underlying_factory(underlying_factory),
   placement_strategy(placement_strategy)
{
}

msess::OrganisingSurfaceFactory::~OrganisingSurfaceFactory()
{
}

std::shared_ptr<msess::Surface> msess::OrganisingSurfaceFactory::create_surface(const msess::SurfaceCreationParameters& params)
{
    auto placed_params = placement_strategy->place(params);

    return underlying_factory->create_surface(placed_params);
}

