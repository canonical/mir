/*
 * Copyright © 2012-2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
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
#include "mir/scene/surface_observers.h"

#include "mir/geometry/rectangle.h"

#include "mir_toolkit/common.h"

#include <glm/glm.hpp>
#include <vector>
#include <list>
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
class Surface;
}
namespace scene
{
class SceneReport;
class CursorStreamImageAdapter;

class BasicSurface : public Surface
{
public:
    BasicSurface(
        std::string const& name,
        geometry::Rectangle rect,
        MirPointerConfinementState state,
        std::list<scene::StreamInfo> const& streams,
        std::shared_ptr<graphics::CursorImage> const& cursor_image,
        std::shared_ptr<SceneReport> const& report);

    BasicSurface(
        std::string const& name,
        geometry::Rectangle rect,
        std::weak_ptr<Surface> const& parent,
        MirPointerConfinementState state,
        std::list<scene::StreamInfo> const& streams,
        std::shared_ptr<graphics::CursorImage> const& cursor_image,
        std::shared_ptr<SceneReport> const& report);

    ~BasicSurface() noexcept;

    std::string name() const override;
    void move_to(geometry::Point const& top_left) override;

    void set_hidden(bool is_hidden);

    geometry::Size size() const override;
    geometry::Size client_size() const override;

    std::shared_ptr<frontend::BufferStream> primary_buffer_stream() const override;
    void set_streams(std::list<scene::StreamInfo> const& streams) override;

    input::InputReceptionMode reception_mode() const override;
    void set_reception_mode(input::InputReceptionMode mode) override;

    void set_input_region(std::vector<geometry::Rectangle> const& input_rectangles) override;

    void resize(geometry::Size const& size) override;
    geometry::Point top_left() const override;
    geometry::Rectangle input_bounds() const override;
    bool input_area_contains(geometry::Point const& point) const override;
    void consume(MirEvent const* event) override;
    void set_alpha(float alpha) override;
    void set_orientation(MirOrientation orientation) override;
    void set_transformation(glm::mat4 const&) override;

    bool visible() const override;
    
    graphics::RenderableList generate_renderables(compositor::CompositorID id) const override;
    int buffers_ready_for_compositor(void const* compositor_id) const override;

    MirWindowType type() const override;
    MirWindowState state() const override;
    int configure(MirWindowAttrib attrib, int value) override;
    int query(MirWindowAttrib attrib) const override;
    void hide() override;
    void show() override;
    
    void set_cursor_image(std::shared_ptr<graphics::CursorImage> const& image) override;
    void remove_cursor_image(); // Removes the cursor image without resetting the stream
    std::shared_ptr<graphics::CursorImage> cursor_image() const override;

    void set_cursor_stream(std::shared_ptr<frontend::BufferStream> const& stream,
                           geometry::Displacement const& hotspot) override;
    void set_cursor_from_buffer(graphics::Buffer& buffer,
                                geometry::Displacement const& hotspot);

    void request_client_surface_close() override;

    std::shared_ptr<Surface> parent() const override;

    void add_observer(std::shared_ptr<SurfaceObserver> const& observer) override;
    void remove_observer(std::weak_ptr<SurfaceObserver> const& observer) override;

    int dpi() const;

    void set_keymap(MirInputDeviceId id, std::string const& model, std::string const& layout,
                    std::string const& variant, std::string const& options) override;

    void rename(std::string const& title) override;

    void set_confine_pointer_state(MirPointerConfinementState state) override;
    MirPointerConfinementState confine_pointer_state() const override;
    void placed_relative(geometry::Rectangle const& placement) override;
    void start_drag_and_drop(std::vector<uint8_t> const& handle) override;

private:
    bool visible(std::unique_lock<std::mutex>&) const;
    MirWindowType set_type(MirWindowType t);  // Use configure() to make public changes
    MirWindowState set_state(MirWindowState s);
    int set_dpi(int);
    MirWindowVisibility set_visibility(MirWindowVisibility v);
    int set_swap_interval(int);
    MirWindowFocusState set_focus_state(MirWindowFocusState f);
    MirOrientationMode set_preferred_orientation(MirOrientationMode mode);

    SurfaceObservers observers;
    std::mutex mutable guard;
    std::string surface_name;
    geometry::Rectangle surface_rect;
    glm::mat4 transformation_matrix;
    float surface_alpha;
    bool hidden;
    input::InputReceptionMode input_mode;
    std::vector<geometry::Rectangle> custom_input_rectangles;
    std::shared_ptr<compositor::BufferStream> const surface_buffer_stream;
    std::shared_ptr<graphics::CursorImage> cursor_image_;
    std::shared_ptr<SceneReport> const report;
    std::weak_ptr<Surface> const parent_;

    std::list<StreamInfo> layers;
    // Surface attributes:
    MirWindowType type_ = mir_window_type_normal;
    MirWindowState state_ = mir_window_state_restored;
    int swapinterval_ = 1;
    MirWindowFocusState focus_ = mir_window_focus_state_unfocused;
    int dpi_ = 0;
    MirWindowVisibility visibility_ = mir_window_visibility_occluded;
    MirOrientationMode pref_orientation_mode = mir_orientation_mode_any;
    MirPointerConfinementState confine_pointer_state_ = mir_pointer_unconfined;

    std::unique_ptr<CursorStreamImageAdapter> const cursor_stream_adapter;
};

}
}

#endif // MIR_SCENE_BASIC_SURFACE_H_
