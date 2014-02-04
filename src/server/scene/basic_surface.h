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

#include "mir_toolkit/common.h"
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
class SurfaceData;
class SceneReport;

class BasicSurface
{
public:
    BasicSurface(std::shared_ptr<SurfaceData> const& surface_data,
        std::shared_ptr<compositor::BufferStream> const& buffer_stream,
        std::shared_ptr<input::InputChannel> const& input_channel,
        std::shared_ptr<SceneReport> const& report);

    virtual ~BasicSurface();

    virtual std::string const& name() const;
    virtual void move_to(geometry::Point const& top_left);
    virtual void set_rotation(float degrees, glm::vec3 const& axis);
    virtual float alpha() const;
    virtual void set_alpha(float alpha);
    virtual void set_hidden(bool is_hidden);

    virtual geometry::Point top_left() const;
    virtual geometry::Size size() const;

    virtual MirPixelFormat pixel_format() const;

    virtual std::shared_ptr<graphics::Buffer> snapshot_buffer() const;
    virtual void swap_buffers(graphics::Buffer*&);
    virtual void force_requests_to_complete();

    virtual bool supports_input() const;
    virtual int client_input_fd() const;
    virtual void allow_framedropping(bool);
    virtual std::shared_ptr<input::InputChannel> input_channel() const;

    virtual void set_input_region(std::vector<geometry::Rectangle> const& input_rectangles);

    virtual std::shared_ptr<compositor::CompositingCriteria> compositing_criteria();

    virtual std::shared_ptr<compositor::BufferStream> buffer_stream() const;

    virtual std::shared_ptr<input::Surface> input_surface() const;

    /**
     * Resize the surface.
     * \returns true if the size changed, false if it was already that size.
     * \throws std::logic_error For impossible sizes like {0,0}.
     */
    virtual bool resize(geometry::Size const& size);

private:
    BasicSurface(BasicSurface const&) = delete;
    BasicSurface& operator=(BasicSurface const&) = delete;

    std::shared_ptr<SurfaceData> surface_data;
    std::shared_ptr<compositor::BufferStream> surface_buffer_stream;
    std::shared_ptr<input::InputChannel> const server_input_channel;
    std::shared_ptr<SceneReport> const report;
};

}
}

#endif // MIR_SCENE_BASIC_SURFACE_H_
