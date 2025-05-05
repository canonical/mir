/*
* Copyright © Canonical Ltd.
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
 */

#ifndef SHELL_SURFACE_H
#define SHELL_SURFACE_H

#include "mir/synchronised.h"
#include "mir/scene/surface.h"
#include "mir/wayland/weak.h"

namespace mir
{
namespace scene
{
/// A [Surface] created by the shell directly, instead of a wayland client.
/// This is useful for instances where you would like to render something,
/// but do not need the overhead of an internal or external client to do so.
class ShellSurface : public Surface
{
public:
    ShellSurface(
        std::string const& name,
        geometry::Rectangle area,
        std::shared_ptr<compositor::BufferStream> const& stream);
    ~ShellSurface() override;

    void register_interest(std::weak_ptr<SurfaceObserver> const& observer) override;
    void register_interest(std::weak_ptr<SurfaceObserver> const& observer, Executor& executor) override;
    void register_early_observer(std::weak_ptr<SurfaceObserver> const& observer, Executor& executor) override;
    void unregister_interest(SurfaceObserver const& observer) override;

    void initial_placement_done() override;
    std::string name() const override;
    void move_to(geometry::Point const& top_left) override;

    void set_hidden(bool is_hidden);

    geometry::Size window_size() const override;

    geometry::Displacement content_offset() const override;
    geometry::Size content_size() const override;

    std::shared_ptr<frontend::BufferStream> primary_buffer_stream() const override;
    void set_streams(std::list<scene::StreamInfo> const& streams) override;

    input::InputReceptionMode reception_mode() const override;
    void set_reception_mode(input::InputReceptionMode mode) override;

    void set_input_region(std::vector<geometry::Rectangle> const& input_rectangles) override;
    std::vector<geometry::Rectangle> get_input_region() const override;

    void resize(geometry::Size const& size) override;
    geometry::Point top_left() const override;
    geometry::Rectangle input_bounds() const override;
    bool input_area_contains(geometry::Point const& point) const override;
    void consume(std::shared_ptr<MirEvent const> const& event) override;
    void set_alpha(float alpha) override;
    void set_orientation(MirOrientation orientation) override;
    void set_transformation(glm::mat4 const&) override;

    bool visible() const override;

    graphics::RenderableList generate_renderables(compositor::CompositorID id) const override;

    MirWindowType type() const override;
    MirWindowState state() const override;
    auto state_tracker() const -> SurfaceStateTracker override;
    int configure(MirWindowAttrib attrib, int value) override;
    int query(MirWindowAttrib attrib) const override;
    void hide() override;
    void show() override;

    void set_cursor_image(std::shared_ptr<graphics::CursorImage> const& image) override;
    void remove_cursor_image(); // Removes the cursor image without resetting the stream
    std::shared_ptr<graphics::CursorImage> cursor_image() const override;

    void set_cursor_from_buffer(std::shared_ptr<graphics::Buffer> buffer,
                                geometry::Displacement const& hotspot);

    auto wayland_surface() -> wayland::Weak<frontend::WlSurface> const& override;

    void request_client_surface_close() override;

    std::shared_ptr<Surface> parent() const override;

    int dpi() const;

    void rename(std::string const& title) override;

    void set_confine_pointer_state(MirPointerConfinementState state) override;
    MirPointerConfinementState confine_pointer_state() const override;
    void placed_relative(geometry::Rectangle const& placement) override;

    auto depth_layer() const -> MirDepthLayer override;
    void set_depth_layer(MirDepthLayer depth_layer) override;

    auto visible_on_lock_screen() const -> bool override;
    void set_visible_on_lock_screen(bool visible) override;

    std::optional<geometry::Rectangle> clip_area() const override;
    void set_clip_area(std::optional<geometry::Rectangle> const& area) override;

    auto focus_state() const -> MirWindowFocusState override;
    void set_focus_state(MirWindowFocusState new_state) override;

    auto application_id() const -> std::string override;
    void set_application_id(std::string const& application_id) override;

    auto session() const -> std::weak_ptr<Session> override;

    void set_window_margins(
        geometry::DeltaY top,
        geometry::DeltaX left,
        geometry::DeltaY bottom,
        geometry::DeltaX right) override;

    auto focus_mode() const -> MirFocusMode override;
    void set_focus_mode(MirFocusMode focus_mode) override;

private:
    class Multiplexer;

    struct State
    {
        std::string surface_name;
        geometry::Rectangle surface_rect;
        glm::mat4 transformation_matrix;
        float surface_alpha;
        bool hidden;
        input::InputReceptionMode input_mode;
        std::shared_ptr<graphics::CursorImage> cursor_image;
        MirWindowType type = mir_window_type_normal;
        SurfaceStateTracker state{mir_window_state_restored};
        MirWindowFocusState focus = mir_window_focus_state_unfocused;
        int dpi = 0;
        MirWindowVisibility visibility = mir_window_visibility_occluded;
        MirOrientationMode pref_orientation_mode = mir_orientation_mode_any;
        MirPointerConfinementState confine_pointer_state = mir_pointer_unconfined;
        MirDepthLayer depth_layer = mir_depth_layer_application;
        bool visible_on_lock_screen = false;
        std::string application_id;
        MirFocusMode focus_mode = mir_focus_mode_focusable;
    };

    MirWindowType set_type(MirWindowType t);  // Use configure() to make public changes
    MirWindowState set_state(MirWindowState s);
    int set_dpi(int);
    MirWindowVisibility set_visibility(MirWindowVisibility v);
    MirOrientationMode set_preferred_orientation(MirOrientationMode mode);

    std::unique_ptr<Multiplexer> observers;
    Synchronised<State> synchronised_state;
    std::shared_ptr<compositor::BufferStream> const stream;

    /// An always weak wayland surface.
    wayland::Weak<frontend::WlSurface> const wayland_surface_;
};
};
}




#endif //SHELL_SURFACE_H
