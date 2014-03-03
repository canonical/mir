/*
 * Copyright © 2012-2014 Canonical Ltd.
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

#include "mir/geometry/rectangle.h"
#include "mir/graphics/renderable.h"
#include "mir/input/surface.h"

#include "mutable_surface_state.h"
#include "mir_toolkit/common.h"

#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include <mutex>
#include <string>

namespace mir
{
namespace compositor
{
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
class SceneReport;

class BasicSurface :
    public graphics::Renderable,
    public input::Surface,
    public MutableSurfaceState
{
public:
    BasicSurface(
        std::string const& name,
        geometry::Rectangle rect,
        std::function<void()> change_cb,
        bool nonrectangular,
        std::shared_ptr<compositor::BufferStream> const& buffer_stream,
        std::shared_ptr<input::InputChannel> const& input_channel,
        std::shared_ptr<SceneReport> const& report);

    ~BasicSurface() noexcept;

    std::string const& name() const override;
    void move_to(geometry::Point const& top_left) override;
    float alpha() const override;
    void set_hidden(bool is_hidden) override;

    geometry::Size size() const override;

    MirPixelFormat pixel_format() const;

    std::shared_ptr<graphics::Buffer> snapshot_buffer() const;
    void swap_buffers(graphics::Buffer* old_buffer, std::function<void(graphics::Buffer* new_buffer)> complete);
    void force_requests_to_complete();

    bool supports_input() const;
    int client_input_fd() const;
    void allow_framedropping(bool);
    std::shared_ptr<input::InputChannel> input_channel() const;

    void set_input_region(std::vector<geometry::Rectangle> const& input_rectangles) override;

    std::shared_ptr<compositor::BufferStream> buffer_stream() const;

    /**
     * Resize the surface.
     * \returns true if the size changed, false if it was already that size.
     * \throws std::logic_error For impossible sizes like {0,0}.
     */
    bool resize(geometry::Size const& size) override;
    geometry::Point top_left() const override;
    bool contains(geometry::Point const& point) const override;
    void frame_posted() override;
    void set_alpha(float alpha) override;
    void set_rotation(float degrees, glm::vec3 const&) override;
    glm::mat4 const& transformation() const override;
    bool should_be_rendered_in(geometry::Rectangle const& rect) const  override;
    bool shaped() const  override;  // meaning the pixel format has alpha

    // Renderable interface
    std::shared_ptr<graphics::Buffer> buffer(unsigned long) const override;
    bool alpha_enabled() const override;
    geometry::Rectangle screen_position() const override;

private:
    BasicSurface(BasicSurface const&) = delete;
    BasicSurface& operator=(BasicSurface const&) = delete;

    std::mutex mutable guard;
    std::function<void()> const notify_change;
    std::string const surface_name;
    geometry::Rectangle surface_rect;
    glm::mat4 rotation_matrix;
    mutable glm::mat4 transformation_matrix;
    mutable geometry::Size transformation_size;
    mutable bool transformation_dirty;
    float surface_alpha;
    bool first_frame_posted;
    bool hidden;
    const bool nonrectangular;
    std::vector<geometry::Rectangle> input_rectangles;
    std::shared_ptr<compositor::BufferStream> const surface_buffer_stream;
    std::shared_ptr<input::InputChannel> const server_input_channel;
    std::shared_ptr<SceneReport> const report;
};

}
}

#endif // MIR_SCENE_BASIC_SURFACE_H_
