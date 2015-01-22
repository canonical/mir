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
#include "mir/basic_observers.h"
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
class InputSender;
class Surface;
}
namespace scene
{
class SceneReport;
class SurfaceConfigurator;

class SurfaceObservers : public SurfaceObserver, BasicObservers<SurfaceObserver>
{
public:
    using BasicObservers<SurfaceObserver>::add;
    using BasicObservers<SurfaceObserver>::remove;

    void attrib_changed(MirSurfaceAttrib attrib, int value) override;
    void resized_to(geometry::Size const& size) override;
    void moved_to(geometry::Point const& top_left) override;
    void hidden_set_to(bool hide) override;
    void frame_posted(int frames_available) override;
    void alpha_set_to(float alpha) override;
    void orientation_set_to(MirOrientation orientation) override;
    void transformation_set_to(glm::mat4 const& t) override;
    void reception_mode_set_to(input::InputReceptionMode mode) override;
    void cursor_image_set_to(graphics::CursorImage const& image) override;
    void client_surface_close_requested() override;
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
        std::shared_ptr<input::InputSender> const& sender,
        std::shared_ptr<SurfaceConfigurator> const& configurator,
        std::shared_ptr<graphics::CursorImage> const& cursor_image,
        std::shared_ptr<SceneReport> const& report);

    BasicSurface(
        std::string const& name,
        geometry::Rectangle rect,
        std::weak_ptr<Surface> const& parent,
        bool nonrectangular,
        std::shared_ptr<compositor::BufferStream> const& buffer_stream,
        std::shared_ptr<input::InputChannel> const& input_channel,
        std::shared_ptr<input::InputSender> const& sender,
        std::shared_ptr<SurfaceConfigurator> const& configurator,
        std::shared_ptr<graphics::CursorImage> const& cursor_image,
        std::shared_ptr<SceneReport> const& report);

    ~BasicSurface() noexcept;

    std::string name() const override;
    void move_to(geometry::Point const& top_left) override;
    float alpha() const override;
    void set_hidden(bool is_hidden);

    geometry::Size size() const override;
    geometry::Size client_size() const override;

    MirPixelFormat pixel_format() const override;

    std::shared_ptr<graphics::Buffer> snapshot_buffer() const;
    void swap_buffers(graphics::Buffer* old_buffer, std::function<void(graphics::Buffer* new_buffer)> complete) override;
    void force_requests_to_complete() override;

    bool supports_input() const override;
    int client_input_fd() const override;
    void allow_framedropping(bool) override;
    std::shared_ptr<input::InputChannel> input_channel() const override;
    input::InputReceptionMode reception_mode() const override;
    void set_reception_mode(input::InputReceptionMode mode) override;

    void set_input_region(std::vector<geometry::Rectangle> const& input_rectangles) override;

    std::shared_ptr<compositor::BufferStream> buffer_stream() const;

    void resize(geometry::Size const& size) override;
    geometry::Point top_left() const override;
    geometry::Rectangle input_bounds() const override;
    bool input_area_contains(geometry::Point const& point) const override;
    void consume(MirEvent const& event) override;
    void set_alpha(float alpha) override;
    void set_orientation(MirOrientation orientation) override;
    void set_transformation(glm::mat4 const&) override;

    bool visible() const override;
    
    std::unique_ptr<graphics::Renderable> compositor_snapshot(void const* compositor_id) const override;

    void with_most_recent_buffer_do(
        std::function<void(graphics::Buffer&)> const& exec) override;

    MirSurfaceType type() const override;
    MirSurfaceState state() const override;
    void take_input_focus(std::shared_ptr<shell::InputTargeter> const& targeter) override;
    int configure(MirSurfaceAttrib attrib, int value) override;
    int query(MirSurfaceAttrib attrib) override;
    void hide() override;
    void show() override;
    
    void set_cursor_image(std::shared_ptr<graphics::CursorImage> const& image) override;
    std::shared_ptr<graphics::CursorImage> cursor_image() const override;

    void request_client_surface_close() override;

    std::shared_ptr<Surface> parent() const override;

    void add_observer(std::shared_ptr<SurfaceObserver> const& observer) override;
    void remove_observer(std::weak_ptr<SurfaceObserver> const& observer) override;

    int dpi() const;

private:
    bool visible(std::unique_lock<std::mutex>&) const;
    MirSurfaceType set_type(MirSurfaceType t);  // Use configure() to make public changes
    MirSurfaceState set_state(MirSurfaceState s);
    int set_dpi(int);
    MirSurfaceVisibility set_visibility(MirSurfaceVisibility v);
    int set_swap_interval(int);
    MirSurfaceFocusState set_focus_state(MirSurfaceFocusState f);
    MirOrientationMode set_preferred_orientation(MirOrientationMode mode);

    SurfaceObservers observers;
    std::mutex mutable guard;
    std::string const surface_name;
    geometry::Rectangle surface_rect;
    glm::mat4 transformation_matrix;
    float surface_alpha;
    bool first_frame_posted;
    bool hidden;
    input::InputReceptionMode input_mode;
    const bool nonrectangular;
    std::vector<geometry::Rectangle> custom_input_rectangles;
    std::shared_ptr<compositor::BufferStream> const surface_buffer_stream;
    std::shared_ptr<input::InputChannel> const server_input_channel;
    std::shared_ptr<input::InputSender> const input_sender;
    std::shared_ptr<SurfaceConfigurator> const configurator;
    std::shared_ptr<graphics::CursorImage> cursor_image_;
    std::shared_ptr<SceneReport> const report;
    std::weak_ptr<Surface> const parent_;

    // Surface attributes:
    MirSurfaceType type_ = mir_surface_type_normal;
    MirSurfaceState state_ = mir_surface_state_restored;
    int swapinterval_ = 1;
    MirSurfaceFocusState focus_ = mir_surface_unfocused;
    int dpi_ = 0;
    MirSurfaceVisibility visibility_ = mir_surface_visibility_exposed;
    MirOrientationMode pref_orientation_mode = mir_orientation_mode_any;
};

}
}

#endif // MIR_SCENE_BASIC_SURFACE_H_
