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

#include "mir/scene/surface.h"
#include "mir/scene/surface_observer.h"

#include "mir/geometry/rectangle.h"

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
namespace frontend { class EventSink; }
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
class SurfaceConfigurator;

class SurfaceObservers : public SurfaceObserver
{
public:

    void attrib_changed(MirSurfaceAttrib attrib, int value) override;
    void resized_to(geometry::Size const& size) override;
    void moved_to(geometry::Point const& top_left) override;
    void hidden_set_to(bool hide) override;
    void frame_posted() override;
    void alpha_set_to(float alpha) override;
    void transformation_set_to(glm::mat4 const& t) override;

    void add(std::shared_ptr<SurfaceObserver> const& observer);
    void remove(std::shared_ptr<SurfaceObserver> const& observer);

private:
    std::mutex mutex;
    std::vector<std::shared_ptr<SurfaceObserver>> observers;
};

class BasicSurface : public Surface
{
public:
    BasicSurface(
        std::string const& name,
        geometry::Rectangle rect,
        bool nonrectangular,
        std::shared_ptr<compositor::BufferStream> const& buffer_stream,
        std::shared_ptr<input::InputChannel> const& input_channel,
        std::shared_ptr<SurfaceConfigurator> const& configurator,
        std::shared_ptr<SceneReport> const& report);

    ~BasicSurface() noexcept;

    std::string name() const override;
    void move_to(geometry::Point const& top_left) override;
    float alpha() const override;
    void set_hidden(bool is_hidden);

    geometry::Size size() const override;

    MirPixelFormat pixel_format() const;

    std::shared_ptr<graphics::Buffer> snapshot_buffer() const;
    void swap_buffers(graphics::Buffer* old_buffer, std::function<void(graphics::Buffer* new_buffer)> complete);
    void force_requests_to_complete();

    bool supports_input() const;
    int client_input_fd() const;
    void allow_framedropping(bool);
    std::shared_ptr<input::InputChannel> input_channel() const override;

    void set_input_region(std::vector<geometry::Rectangle> const& input_rectangles) override;

    std::shared_ptr<compositor::BufferStream> buffer_stream() const;

    void resize(geometry::Size const& size) override;
    geometry::Point top_left() const override;
    bool contains(geometry::Point const& point) const override;
    void set_alpha(float alpha) override;
    void set_transformation(glm::mat4 const&) override;

    bool visible() const;
    
    std::unique_ptr<graphics::Renderable> compositor_snapshot(void const* compositor_id) const;

    void with_most_recent_buffer_do(
        std::function<void(graphics::Buffer&)> const& exec) override;

    MirSurfaceType type() const override;
    MirSurfaceState state() const override;
    void take_input_focus(std::shared_ptr<shell::InputTargeter> const& targeter) override;
    int configure(MirSurfaceAttrib attrib, int value) override;
    void hide() override;
    void show() override;

    void add_observer(std::shared_ptr<SurfaceObserver> const& observer) override;
    void remove_observer(std::weak_ptr<SurfaceObserver> const& observer) override;

private:
    bool visible(std::unique_lock<std::mutex>&) const;
    bool set_type(MirSurfaceType t);  // Use configure() to make public changes
    bool set_state(MirSurfaceState s);

    SurfaceObservers observers;
    std::mutex mutable guard;
    std::string const surface_name;
    geometry::Rectangle surface_rect;
    glm::mat4 transformation_matrix;
    float surface_alpha;
    bool first_frame_posted;
    bool hidden;
    const bool nonrectangular;
    std::vector<geometry::Rectangle> input_rectangles;
    std::shared_ptr<compositor::BufferStream> const surface_buffer_stream;
    std::shared_ptr<input::InputChannel> const server_input_channel;
    std::shared_ptr<SurfaceConfigurator> const configurator;
    std::shared_ptr<SceneReport> const report;

    MirSurfaceType type_value;
    MirSurfaceState state_value;
};

}
}

#endif // MIR_SCENE_BASIC_SURFACE_H_
