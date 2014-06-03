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
 * Authored By: Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef MIR_SHELL_SURFACE_COORDINATOR_WRAPPER_H_
#define MIR_SHELL_SURFACE_COORDINATOR_WRAPPER_H_

#include "mir/scene/surface_coordinator.h"

namespace mir
{
namespace shell
{
class SurfaceCoordinatorWrapper : public scene::SurfaceCoordinator
{
public:
    explicit SurfaceCoordinatorWrapper(std::shared_ptr<scene::SurfaceCoordinator> const& wrapped);

    std::shared_ptr<scene::Surface> add_surface(
        scene::SurfaceCreationParameters const& params,
        scene::Session* session) override;

    void raise(std::weak_ptr<scene::Surface> const& surface) override;

    void remove_surface(std::weak_ptr<scene::Surface> const& surface) override;

protected:
    std::shared_ptr<SurfaceCoordinator> const wrapped;
};
}
}

#endif /* MIR_SHELL_SURFACE_COORDINATOR_WRAPPER_H_ */
