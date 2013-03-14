/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Thomas Voss <thomas.voss@canonical.com>
 */

#ifndef MIR_SURFACES_SURFACE_CONTROLLER_H_
#define MIR_SURFACES_SURFACE_CONTROLLER_H_

#include "mir/shell/surface_factory.h"

#include <memory>

namespace mir
{

/// Management of Surface objects. Includes the model (SurfaceStack and Surface
/// classes) and controller (SurfaceController) elements of an MVC design.
namespace surfaces
{

class Surface;
class SurfaceStackModel;

class SurfaceController : public shell::SurfaceFactory
{
public:
    explicit SurfaceController(std::shared_ptr<SurfaceStackModel> const& surface_stack);
    virtual ~SurfaceController() {}

    std::shared_ptr<shell::Surface> create_surface(const shell::SurfaceCreationParameters& params);

protected:
    SurfaceController(const SurfaceController&) = delete;
    SurfaceController& operator=(const SurfaceController&) = delete;

private:
    std::shared_ptr<SurfaceStackModel> const surface_stack;
};

}
}

#endif // MIR_SURFACES_SURFACE_CONTROLLER_H_
