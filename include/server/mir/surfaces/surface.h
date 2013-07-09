/*
 * Copyright © 2012 Canonical Ltd.
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
 * Authored by: Thomas Voss <thomas.voss@canonical.com>
 */

#ifndef MIR_SURFACES_SURFACE_H_
#define MIR_SURFACES_SURFACE_H_

#include "mir/geometry/pixel_format.h"
#include "mir/geometry/rectangle.h"
#include "mir/graphics/renderable.h"
#include "mir/compositor/buffer_properties.h"

#include <vector>
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
namespace input
{
class InputChannel;
class InputRegion;
class SurfaceInfoController;
}

namespace surfaces
{
class SurfaceInfoController;
class BufferStream;

// TODO this is ideally an implementation class. It is only in a public header
// TODO because it is used in some example code (which probably needs rethinking).
class Surface : public graphics::Renderable
{
public:
    //TODO: surfaces::SurfaceInfo shouldn't be here, rather, something like mg::SurfaceInfo
    Surface(std::shared_ptr<surfaces::SurfaceInfoController> const& basic_info,
            std::shared_ptr<input::SurfaceInfoController> const& input_info,
            std::shared_ptr<BufferStream> buffer_stream,
            std::shared_ptr<input::InputChannel> const& input_channel,
            std::function<void()> const& change_callback);

    ~Surface();

    std::string const& name() const;
    void move_to(geometry::Point const& top_left);
    void set_rotation(float degrees, glm::vec3 const& axis);
    void set_alpha(float alpha);
    void set_hidden(bool is_hidden);

    /* From Renderable */
    geometry::Point top_left() const;
    geometry::Size size() const;
    std::shared_ptr<GraphicRegion> graphic_region() const;
    const glm::mat4& transformation() const override;
    float alpha() const;
    bool should_be_rendered() const;

    geometry::PixelFormat pixel_format() const;

    std::shared_ptr<compositor::Buffer> compositor_buffer() const;
    std::shared_ptr<compositor::Buffer> advance_client_buffer();
    void force_requests_to_complete();
    void flag_for_render();

    bool supports_input() const;
    int client_input_fd() const;
    void allow_framedropping(bool);
    std::shared_ptr<input::InputChannel> input_channel() const;

    void set_input_region(std::vector<geometry::Rectangle> const& input_rectangles);

private:
    //TODO: surfaces::SurfaceInfo shouldn't be here, rather, something like mg::SurfaceInfo
    std::shared_ptr<surfaces::SurfaceInfoController> basic_info;
    std::shared_ptr<input::SurfaceInfoController> input_info;

    std::shared_ptr<BufferStream> buffer_stream;

    std::shared_ptr<input::InputChannel> const server_input_channel;

    glm::mat4 rotation_matrix;
    mutable glm::mat4 transformation_matrix;
    mutable geometry::Size transformation_size;
    mutable bool transformation_dirty;
    float alpha_value;

    bool is_hidden;
    unsigned int buffer_count;
    std::function<void()> notify_change;
};

}
}

#endif // MIR_SURFACES_SURFACE_H_
