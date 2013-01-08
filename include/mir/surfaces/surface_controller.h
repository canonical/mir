/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Thomas Voss <thomas.voss@canonical.com>
 */

#ifndef MIR_SURFACES_SURFACE_CONTROLLER_H_
#define MIR_SURFACES_SURFACE_CONTROLLER_H_

#include "mir/sessions/surface_organiser.h"

#include <memory>

namespace mir
{

/// Management of Surface objects. Includes the model (SurfaceStack and Surface
/// classes) and controller (SurfaceController) elements of an MVC design.
namespace surfaces
{

class Surface;
class SurfaceStackModel;

class SurfaceController : public sessions::SurfaceOrganiser
{
public:
    explicit SurfaceController(SurfaceStackModel* surface_stack);
    virtual ~SurfaceController() {}

    std::shared_ptr<sessions::Surface> create_surface(const sessions::SurfaceCreationParameters& params);

    void destroy_surface(std::shared_ptr<sessions::Surface> const& surface);

    void hide_surface(std::shared_ptr<sessions::Surface> const& surface);
    void show_surface(std::shared_ptr<sessions::Surface> const& surface);

protected:
    SurfaceController(const SurfaceController&) = delete;
    SurfaceController& operator=(const SurfaceController&) = delete;

private:
    SurfaceStackModel* const surface_stack;
};

}
}

#endif // MIR_SURFACES_SURFACE_CONTROLLER_H_
