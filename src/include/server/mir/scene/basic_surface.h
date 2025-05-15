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

#ifndef MIR_SCENE_BASIC_SURFACE_H_
#define MIR_SCENE_BASIC_SURFACE_H_

#include "mir/graphics/display_configuration.h"
#include "mir/scene/surface.h"
#include "mir/wayland/weak.h"
#include "mir/geometry/rectangle.h"
#include "mir_toolkit/common.h"
#include "mir/synchronised.h"

#include <glm/glm.hpp>
#include <vector>
#include <list>
#include <memory>
#include <string>

namespace mir
{
namespace compositor
{
class BufferStream;
}
namespace frontend { class EventSink; }
namespace graphics
{
class Buffer;
class DisplayConfigurationObserver;
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
        std::shared_ptr<SceneReport> const& report,
        std::shared_ptr<ObserverRegistrar<graphics::DisplayConfigurationObserver>> const& display_config_registrar);

    BasicSurface(
        std::string const& name,
        geometry::Rectangle rect,
        std::weak_ptr<Surface> const& parent,
        MirPointerConfinementState state,
        std::list<scene::StreamInfo> const& streams,
        std::shared_ptr<graphics::CursorImage> const& cursor_image,
        std::shared_ptr<SceneReport> const& report,
        std::shared_ptr<ObserverRegistrar<graphics::DisplayConfigurationObserver>> const& display_config_registrar);

    ~BasicSurface() noexcept;

    void register_interest(std::weak_ptr<SurfaceObserver> const& observer) override;
    void register_interest(std::weak_ptr<SurfaceObserver> const& observer, Executor& executor) override;
    void register_early_observer(std::weak_ptr<SurfaceObserver> const& observer, Executor& executor) override;
    void unregister_interest(SurfaceObserver const& observer) override;

    virtual void initial_placement_done() override;
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

    Flags<MirTiledEdge> tiled_edges() const override;
    void set_tiled_edges(Flags<MirTiledEdge> flags) override;

private:
    struct State;
    class DisplayConfigurationEarlyListener;
    class Multiplexer;

    bool visible(State const& state) const;
    MirWindowType set_type(MirWindowType t);  // Use configure() to make public changes
    MirWindowState set_state(MirWindowState s);
    int set_dpi(int);
    MirWindowVisibility set_visibility(MirWindowVisibility v);
    MirOrientationMode set_preferred_orientation(MirOrientationMode mode);
    void clear_frame_posted_callbacks(State& state);
    void update_frame_posted_callbacks(State& state);
    auto content_size(State const& state) const -> geometry::Size;
    auto content_top_left(State const& state) const -> geometry::Point;
    void track_outputs();
    void linearised_track_outputs();

    struct State
    {
        std::string surface_name;
        geometry::Rectangle surface_rect;
        glm::mat4 transformation_matrix;
        float surface_alpha;
        bool hidden;
        input::InputReceptionMode input_mode;
        std::vector<geometry::Rectangle> custom_input_rectangles{};
        std::shared_ptr<graphics::CursorImage> cursor_image;

        std::list<StreamInfo> layers;
        // Surface attributes:
        MirWindowType type = mir_window_type_normal;
        SurfaceStateTracker state{mir_window_state_restored};
        int swap_interval = 1;
        MirWindowFocusState focus = mir_window_focus_state_unfocused;
        int dpi = 0;
        MirWindowVisibility visibility = mir_window_visibility_occluded;
        MirOrientationMode pref_orientation_mode = mir_orientation_mode_any;
        MirPointerConfinementState confine_pointer_state = mir_pointer_unconfined;
        MirDepthLayer depth_layer = mir_depth_layer_application;
        bool visible_on_lock_screen = false;

        std::optional<geometry::Rectangle> clip_area{std::nullopt};
        std::string application_id{""};

        struct {
            geometry::DeltaY top;
            geometry::DeltaX left;
            geometry::DeltaY bottom;
            geometry::DeltaX right;
        } margins{};

        MirFocusMode focus_mode = mir_focus_mode_focusable;
        Flags<MirTiledEdge> tiled_edges = Flags(mir_tiled_edge_none);
    };
    mir::Synchronised<State> synchronised_state;

    std::shared_ptr<Multiplexer> const observers;
    std::shared_ptr<compositor::BufferStream> const surface_buffer_stream;
    std::shared_ptr<SceneReport> const report;
    std::weak_ptr<Surface> const parent_;
    std::shared_ptr<ObserverRegistrar<graphics::DisplayConfigurationObserver>> display_config_registrar;
    std::shared_ptr<DisplayConfigurationEarlyListener> const display_config_monitor;
    std::shared_ptr<graphics::DisplayConfiguration const> display_config;
    std::unordered_map<graphics::DisplayConfigurationOutputId, float> tracked_output_scales;
    wayland::Weak<frontend::WlSurface> weak_surface;
};

}
}

#endif // MIR_SCENE_BASIC_SURFACE_H_
