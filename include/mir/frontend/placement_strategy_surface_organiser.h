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

#ifndef MIR_FRONTEND_PLACEMENT_STRATEGY_SURFACE_ORGANISER_H_
#define MIR_FRONTEND_PLACEMENT_STRATEGY_SURFACE_ORGANISER_H_

#include "mir/frontend/surface_organiser.h"

#include <memory>

namespace mir
{
namespace frontend
{
class PlacementStrategy;

class PlacementStrategySurfaceOrganiser : public SurfaceOrganiser
{
public:
    PlacementStrategySurfaceOrganiser(std::shared_ptr<SurfaceOrganiser> const& underlying_organiser,
                                      std::shared_ptr<PlacementStrategy> const& placement_strategy);
    virtual ~PlacementStrategySurfaceOrganiser() {}
    
    std::weak_ptr<surfaces::Surface> create_surface(const surfaces::SurfaceCreationParameters& params);
    void destroy_surface(std::weak_ptr<surfaces::Surface> const& surface);

    void hide_surface(std::weak_ptr<surfaces::Surface> const& surface);
    void show_surface(std::weak_ptr<surfaces::Surface> const& surface);

protected:
    PlacementStrategySurfaceOrganiser(const PlacementStrategySurfaceOrganiser&) = delete;
    PlacementStrategySurfaceOrganiser& operator=(const PlacementStrategySurfaceOrganiser&) = delete;

private:
    std::shared_ptr<SurfaceOrganiser> underlying_organiser;
    std::shared_ptr<PlacementStrategy> placement_strategy;
};

}
} // namespace mir

#endif // MIR_FRONTEND_PLACEMENT_STRATEGY_SURFACE_ORGANISER_H_
