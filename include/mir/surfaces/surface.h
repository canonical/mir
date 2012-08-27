/*
 * Copyright © 2012 Canonical Ltd.
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
 * Authored by: Thomas Voss <thomas.voss@canonical.com>
 */

#ifndef MIR_SURFACES_SURFACE_H_
#define MIR_SURFACES_SURFACE_H_

#include "mir/geometry/dimensions.h"
#include "mir/graphics/renderable.h"

#include <memory>

namespace mir
{
namespace compositor
{

class BufferTextureBinder;

}
namespace surfaces
{

struct SurfaceCreationParameters
{
    SurfaceCreationParameters& of_width(geometry::Width new_width);

    SurfaceCreationParameters& of_height(geometry::Height new_height);

    SurfaceCreationParameters& of_size(geometry::Width new_width, geometry::Height new_height);

    geometry::Width width;
    geometry::Height height;
};

bool operator==(const SurfaceCreationParameters& lhs, const SurfaceCreationParameters& rhs);
bool operator!=(const SurfaceCreationParameters& lhs, const SurfaceCreationParameters& rhs);


SurfaceCreationParameters a_surface();

class Surface : public graphics::Renderable
{
 public:
    Surface(const SurfaceCreationParameters& params,
            std::shared_ptr<compositor::BufferTextureBinder> buffer_texture_binder);

    geometry::Width width() const { return geometry::Width(); }
    geometry::Height height() const { return geometry::Height(); }
 private:
    std::shared_ptr<compositor::BufferTextureBinder> buffer_texture_binder;
};

}
}

#endif // MIR_SURFACES_SURFACE_H_
