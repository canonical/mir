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
#include "mir/compositor/buffer_properties.h"

#include <memory>
#include <string>

namespace mir
{
namespace compositor
{
class BufferBundle;
class GraphicBufferClientResource;
class GraphicBufferCompositorResource;
class GraphicRegion;
struct BufferIPCPackage;
class BufferID;
}

namespace surfaces
{

struct SurfaceCreationParameters
{
    SurfaceCreationParameters();

    SurfaceCreationParameters& of_name(std::string const& new_name);

    SurfaceCreationParameters& of_size(geometry::Size new_size);

    SurfaceCreationParameters& of_size(geometry::Width::ValueType width, geometry::Height::ValueType height);

    SurfaceCreationParameters& of_buffer_usage(compositor::BufferUsage new_buffer_usage);

    SurfaceCreationParameters& of_pixel_format(geometry::PixelFormat new_pixel_format);

    std::string name;
    geometry::Size size;
    compositor::BufferUsage buffer_usage;
    geometry::PixelFormat pixel_format;
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
    void move_to(geometry::Point const& top_left);
    void set_rotation(float degrees, glm::vec3 const& axis);
    void set_alpha(float alpha);

    /* From Renderable */
    geometry::Point top_left() const;
    geometry::Size size() const;
    std::shared_ptr<compositor::GraphicBufferCompositorResource> texture() const;
    glm::mat4 transformation() const;
    float alpha() const;
    bool hidden() const;
    void set_hidden(bool is_hidden);

    geometry::PixelFormat pixel_format() const;

    void advance_client_buffer();
    std::shared_ptr<compositor::GraphicBufferClientResource> client_buffer_resource() const;
    void shutdown();

 private:
    std::string surface_name;
    std::shared_ptr<compositor::BufferBundle> buffer_bundle;
    std::shared_ptr<compositor::GraphicBufferClientResource> graphics_resource;
    geometry::Point top_left_point;
    glm::mat4 transformation_matrix;
    float alpha_value;

    bool is_hidden;
};

}
}

#endif // MIR_SURFACES_SURFACE_H_
