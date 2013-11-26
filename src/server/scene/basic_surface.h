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

#ifndef MIR_SCENE_BASIC_SURFACE_H_
#define MIR_SCENE_BASIC_SURFACE_H_

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
namespace scene
{
class BasicSurface
{
public:
    BasicSurface() = default;
    virtual ~BasicSurface() = default;

    virtual std::string const& name() const = 0;
    virtual void move_to(geometry::Point const& top_left) = 0;
    virtual void set_rotation(float degrees, glm::vec3 const& axis) = 0;
    virtual void set_alpha(float alpha) = 0;
    virtual void set_hidden(bool is_hidden) = 0;

    virtual geometry::Point top_left() const = 0;
    virtual geometry::Size size() const = 0;

    virtual geometry::PixelFormat pixel_format() const = 0;

    virtual std::shared_ptr<graphics::Buffer> snapshot_buffer() const = 0;
    virtual void swap_buffers(std::shared_ptr<graphics::Buffer>&) = 0;
    virtual void force_requests_to_complete() = 0;

    virtual bool supports_input() const = 0;
    virtual int client_input_fd() const = 0;
    virtual void allow_framedropping(bool) = 0;
    virtual std::shared_ptr<input::InputChannel> input_channel() const = 0;

    virtual void set_input_region(std::vector<geometry::Rectangle> const& input_rectangles) = 0;

    virtual std::shared_ptr<compositor::CompositingCriteria> compositing_criteria() = 0;

    virtual std::shared_ptr<compositor::BufferStream> buffer_stream() const = 0;

    virtual std::shared_ptr<input::Surface> input_surface() const = 0;
    virtual void resize(geometry::Size const& size) = 0;

private:
    BasicSurface(BasicSurface const&) = delete;
    BasicSurface& operator=(BasicSurface const&) = delete;
};

}
}

#endif // MIR_SCENE_BASIC_SURFACE_H_
