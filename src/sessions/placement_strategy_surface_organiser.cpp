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

#include "mir/sessions/placement_strategy_surface_organiser.h"
#include "mir/sessions/placement_strategy.h"

namespace msess = mir::sessions;
namespace ms = mir::surfaces;

msess::PlacementStrategySurfaceOrganiser::PlacementStrategySurfaceOrganiser(std::shared_ptr<msess::SurfaceOrganiser> const& underlying_organiser,
                                                                         std::shared_ptr<msess::PlacementStrategy> const& placement_strategy)
 : underlying_organiser(underlying_organiser),
   placement_strategy(placement_strategy)
{
}

msess::PlacementStrategySurfaceOrganiser::~PlacementStrategySurfaceOrganiser()
{
}

std::weak_ptr<ms::Surface> msess::PlacementStrategySurfaceOrganiser::create_surface(const ms::SurfaceCreationParameters& params)
{
    ms::SurfaceCreationParameters placed_params;

    placement_strategy->place(params, placed_params);

    return underlying_organiser->create_surface(placed_params);
}

void msess::PlacementStrategySurfaceOrganiser::destroy_surface(std::weak_ptr<ms::Surface> const& surface)
{
    underlying_organiser->destroy_surface(surface);
}

void msess::PlacementStrategySurfaceOrganiser::hide_surface(std::weak_ptr<ms::Surface> const& surface)
{
    underlying_organiser->hide_surface(surface);
}

void msess::PlacementStrategySurfaceOrganiser::show_surface(std::weak_ptr<ms::Surface> const& surface)
{
    underlying_organiser->show_surface(surface);
}
