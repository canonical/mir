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

#include "mir/frontend/placement_strategy_surface_organiser.h"
#include "mir/frontend/placement_strategy.h"

namespace mf = mir::frontend;
namespace ms = mir::surfaces;

mf::PlacementStrategySurfaceOrganiser::PlacementStrategySurfaceOrganiser(std::shared_ptr<mf::SurfaceOrganiser> const& underlying_organiser,
                                                                         std::shared_ptr<mf::PlacementStrategy> const& placement_strategy)
 : underlying_organiser(underlying_organiser),
   placement_strategy(placement_strategy)
{
}

mf::PlacementStrategySurfaceOrganiser::~PlacementStrategySurfaceOrganiser()
{
}

std::weak_ptr<ms::Surface> mf::PlacementStrategySurfaceOrganiser::create_surface(const ms::SurfaceCreationParameters& params)
{
    (void)params;
    return std::weak_ptr<ms::Surface>();
}

void mf::PlacementStrategySurfaceOrganiser::destroy_surface(std::weak_ptr<ms::Surface> const& surface)
{
    (void)surface;
}

void mf::PlacementStrategySurfaceOrganiser::hide_surface(std::weak_ptr<ms::Surface> const& surface)
{
    (void)surface;
}

void mf::PlacementStrategySurfaceOrganiser::show_surface(std::weak_ptr<ms::Surface> const& surface)
{
    (void)surface;
}
