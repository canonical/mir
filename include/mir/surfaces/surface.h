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

#include "mir/geometry/pixel_format.h"
#include "mir/graphics/renderable.h"

#include <memory>
#include <string>

namespace mir
{
namespace compositor
{
class BufferBundle;
class GraphicBufferClientResource;
struct BufferIPCPackage;
}

namespace surfaces
{

struct SurfaceCreationParameters
{
    SurfaceCreationParameters& of_name(std::string const& new_name);

    SurfaceCreationParameters& of_size(geometry::Size new_size);

    std::string name;
    geometry::Size size;
};

bool operator==(const SurfaceCreationParameters& lhs, const SurfaceCreationParameters& rhs);
bool operator!=(const SurfaceCreationParameters& lhs, const SurfaceCreationParameters& rhs);


SurfaceCreationParameters a_surface();

class Surface : public graphics::Renderable
{
 public:
    Surface(const std::string& name,
            std::shared_ptr<compositor::BufferBundle> buffer_bundle);

    ~Surface();

    std::string const& name() const;

    /* From Renderable */
    geometry::Point top_left() const;
    geometry::Size size() const;
    std::shared_ptr<graphics::Texture> texture() const;
    glm::mat4 transformation() const;

    geometry::PixelFormat pixel_format() const;
    std::shared_ptr<compositor::BufferIPCPackage> get_buffer_ipc_package();

 private:
    std::string surface_name;
    std::shared_ptr<compositor::BufferBundle> buffer_bundle;
    std::shared_ptr<compositor::GraphicBufferClientResource> graphics_resource;
    geometry::Point top_left_point;
    glm::mat4 transformation_matrix;
};

}
}

#endif // MIR_SURFACES_SURFACE_H_
