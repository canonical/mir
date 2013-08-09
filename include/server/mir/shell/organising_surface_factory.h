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

#ifndef MIR_SHELL_ORGANISING_SURFACE_FACTORY_H_
#define MIR_SHELL_ORGANISING_SURFACE_FACTORY_H_

#include "mir/shell/surface_factory.h"

#include <memory>

namespace mir
{
namespace shell
{
class PlacementStrategy;
class Session;

class OrganisingSurfaceFactory : public SurfaceFactory
{
public:
    OrganisingSurfaceFactory(std::shared_ptr<SurfaceFactory> const& underlying_factory,
                             std::shared_ptr<PlacementStrategy> const& placement_strategy);
    virtual ~OrganisingSurfaceFactory();

    std::shared_ptr<Surface> create_surface(
        Session* session,
        shell::SurfaceCreationParameters const& params,
        frontend::SurfaceId id,
        std::shared_ptr<frontend::EventSink> const& sink) override;

protected:
    OrganisingSurfaceFactory(OrganisingSurfaceFactory const&) = delete;
    OrganisingSurfaceFactory& operator=(OrganisingSurfaceFactory const&) = delete;

private:
    std::shared_ptr<SurfaceFactory> const underlying_factory;
    std::shared_ptr<PlacementStrategy> const placement_strategy;
};

}
} // namespace mir

#endif // MIR_SHELL_ORGANISING_SURFACE_FACTORY_H_
