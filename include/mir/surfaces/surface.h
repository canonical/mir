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

#include <cassert>
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
    SurfaceCreationParameters(
        geometry::Width w = geometry::Width(0),
        geometry::Height h = geometry::Height(0));

    SurfaceCreationParameters& of_width(geometry::Width new_width);

    SurfaceCreationParameters& of_height(geometry::Height new_height);

    SurfaceCreationParameters& of_size(geometry::Width new_width, geometry::Height new_height);

    bool operator==(const SurfaceCreationParameters& rhs) const;
    
    geometry::Width width;
    geometry::Height height;    
};

SurfaceCreationParameters a_surface();

class Surface
{
 public:
    Surface(const SurfaceCreationParameters& /*params*/,
            std::shared_ptr<compositor::BufferTextureBinder> buffer_texture_binder);

 private:
    std::shared_ptr<compositor::BufferTextureBinder> buffer_texture_binder;
};
    
}
}

#endif // MIR_SURFACES_SURFACE_H_
