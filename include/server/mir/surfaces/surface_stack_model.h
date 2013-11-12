/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
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

#ifndef MIR_SURFACES_SURFACE_STACK_MODEL_H_
#define MIR_SURFACES_SURFACE_STACK_MODEL_H_

#include "mir/surfaces/depth_id.h"

#include <memory>

namespace mir
{
namespace shell
{
struct SurfaceCreationParameters;
}

namespace surfaces
{

class Surface;

class SurfaceStackModel
{
public:
    virtual ~SurfaceStackModel() {}

    virtual std::weak_ptr<Surface> create_surface(shell::SurfaceCreationParameters const& params) = 0;
    virtual void destroy_surface(std::weak_ptr<Surface> const& surface) = 0;

    virtual void raise(std::weak_ptr<Surface> const& surface) = 0;

protected:
    SurfaceStackModel() = default;
    SurfaceStackModel(const SurfaceStackModel&) = delete;
    SurfaceStackModel& operator=(const SurfaceStackModel&) = delete;
};

}
}

#endif // MIR_SURFACES_SURFACE_STACK_MODEL_H_
