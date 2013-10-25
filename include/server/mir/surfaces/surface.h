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
#include "mir/graphics/buffer_properties.h"

#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include <string>

namespace mir
{
namespace compositor
{
class CompositingCriteria;
struct BufferIPCPackage;
class BufferStream;
}
namespace graphics
{
class Buffer;
}
namespace input
{
class InputChannel;
class Surface;
}
namespace surfaces
{
class SurfaceState;
class SurfacesReport;

// TODO this is ideally an implementation class. It is only in a public header
// TODO because it is used in some example code (which probably needs rethinking).
class Surface
{
public:
    Surface(std::shared_ptr<surfaces::SurfaceState> const& surface_state,
            std::shared_ptr<compositor::BufferStream> const& buffer_stream,
            std::shared_ptr<input::InputChannel> const& input_channel,
            std::shared_ptr<SurfacesReport> const& report);

    ~Surface();

    std::string const& name() const;
    void move_to(geometry::Point const& top_left);
    void set_rotation(float degrees, glm::vec3 const& axis);
    void set_alpha(float alpha);
    void set_hidden(bool is_hidden);

    /* From Renderable */
    geometry::Point top_left() const;
    geometry::Size size() const;

    geometry::PixelFormat pixel_format() const;

    std::shared_ptr<graphics::Buffer> snapshot_buffer() const;
    std::shared_ptr<graphics::Buffer> advance_client_buffer();
    void force_requests_to_complete();
    void flag_for_render();

    bool supports_input() const;
    int client_input_fd() const;
    void allow_framedropping(bool);
    std::shared_ptr<input::InputChannel> input_channel() const;

    void set_input_region(std::vector<geometry::Rectangle> const& input_rectangles);

    std::shared_ptr<compositor::CompositingCriteria> compositing_criteria();

    std::shared_ptr<compositor::BufferStream> buffer_stream() const;

    std::shared_ptr<input::Surface> input_surface() const;
private:
    std::shared_ptr<surfaces::SurfaceState> surface_state;
    std::shared_ptr<compositor::BufferStream> surface_buffer_stream;
    std::shared_ptr<input::InputChannel> const server_input_channel;
    std::shared_ptr<SurfacesReport> const report;
    bool surface_in_startup;
};

}
}

#endif // MIR_SURFACES_SURFACE_H_
