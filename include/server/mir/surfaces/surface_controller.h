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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */


#ifndef MIR_SURFACES_SURFACE_CONTROLLER_H_
#define MIR_SURFACES_SURFACE_CONTROLLER_H_

#include "mir/shell/surface_builder.h"
#include "mir/shell/surface_controller.h"

namespace mir
{
namespace shell
{
class Session;
}

namespace surfaces
{
class SurfaceStackModel;

/// Will grow up to provide synchronization of model updates
class SurfaceController : public shell::SurfaceBuilder, public shell::SurfaceController
{
public:
    explicit SurfaceController(std::shared_ptr<SurfaceStackModel> const& surface_stack);

    virtual std::weak_ptr<Surface> create_surface(shell::Session* session, shell::SurfaceCreationParameters const& params);
    virtual void destroy_surface(std::weak_ptr<Surface> const& surface);

    virtual void raise(std::weak_ptr<Surface> const& surface);

private:
    std::shared_ptr<SurfaceStackModel> const surface_stack;
};

}
}


#endif /* MIR_SURFACES_SURFACE_CONTROLLER_H_ */
