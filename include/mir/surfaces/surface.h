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
#include "mir/compositor/pixel_format.h"

#include <memory>
#include <string>

namespace mir
{
namespace compositor
{
class BufferTextureBinder;
struct BufferIPCPackage;
}

namespace surfaces
{

struct SurfaceCreationParameters
{
    SurfaceCreationParameters& of_name(std::string const& new_name);

    SurfaceCreationParameters& of_width(geometry::Width new_width);

    SurfaceCreationParameters& of_height(geometry::Height new_height);

    SurfaceCreationParameters& of_size(geometry::Width new_width, geometry::Height new_height);

    std::string name;
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

    std::string const& name() const;
    geometry::Width width() const;
    geometry::Height height() const;
    compositor::PixelFormat pixel_format() const;
    std::shared_ptr<compositor::BufferIPCPackage> get_current_buffer_ipc_package() const;
    std::shared_ptr<compositor::BufferIPCPackage> get_next_buffer_ipc_package() const;

 private:
    SurfaceCreationParameters params;
    std::shared_ptr<compositor::BufferTextureBinder> buffer_texture_binder;
};

}
}

#endif // MIR_SURFACES_SURFACE_H_
