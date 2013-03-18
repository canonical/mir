/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
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
class Buffer;
class GraphicRegion;
struct BufferIPCPackage;
class BufferID;
}

namespace surfaces
{
class BufferBundle;

// TODO this is ideally an implementation class. It is only in a public header
// TODO because it is used in some example code (which probably needs rethinking).
class Surface : public graphics::Renderable
{
public:
    Surface(const std::string& name, std::shared_ptr<BufferBundle> buffer_bundle);

    ~Surface();

    std::string const& name() const;
    void move_to(geometry::Point const& top_left);
    void set_rotation(float degrees, glm::vec3 const& axis);
    void set_alpha(float alpha);

    /* From Renderable */
    geometry::Point top_left() const;
    geometry::Size size() const;
    std::shared_ptr<GraphicRegion> graphic_region() const;
    glm::mat4 transformation() const;
    float alpha() const;
    bool hidden() const;
    void set_hidden(bool is_hidden);

    geometry::PixelFormat pixel_format() const;

    // TODO client code always (and necessarily) calls advance_client_buffer()
    // TODO and then client_buffer(). That's a bad interface.
    void advance_client_buffer();
    std::shared_ptr<compositor::Buffer> client_buffer() const;
    void shutdown();

private:
    std::string surface_name;
    std::shared_ptr<BufferBundle> buffer_bundle;

    std::shared_ptr<compositor::Buffer> client_buffer_resource;
    geometry::Point top_left_point;
    glm::mat4 transformation_matrix;
    float alpha_value;

    bool is_hidden;
};

}
}

#endif // MIR_SURFACES_SURFACE_H_
