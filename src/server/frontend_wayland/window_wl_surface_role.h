/*
 * Copyright © Canonical Ltd.
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
 */

#ifndef MIR_FRONTEND_WINDOW_WL_SURFACE_ROLE_H
#define MIR_FRONTEND_WINDOW_WL_SURFACE_ROLE_H

#include "wl_surface_role.h"

#include "mir/wayland/weak.h"
#include "mir/wayland/lifetime_tracker.h"
#include "mir/geometry/displacement.h"
#include "mir/geometry/size.h"
#include "mir/geometry/rectangle.h"
#include "mir/graphics/display_configuration.h"

#include <mir_toolkit/common.h>

#include <optional>
#include <chrono>

struct wl_client;
struct wl_resource;
struct MirInputEvent;

namespace mir
{
class Executor;
namespace scene
{
class Surface;
class Session;
}
namespace shell
{
struct SurfaceSpecification;
class Shell;
}
namespace wayland
{
class Client;
}
namespace frontend
{
class WaylandSurfaceObserver;
class OutputManager;
class WlSurface;
class WlSeat;

class WindowWlSurfaceRole
    : public WlSurfaceRole,
      public virtual wayland::LifetimeTracker
{
public:
    WindowWlSurfaceRole(
        Executor& wayland_executor,
        WlSeat* seat,
        wayland::Client* client,
        WlSurface* surface,
        std::shared_ptr<shell::Shell> const& shell,
        OutputManager* output_manager);

    ~WindowWlSurfaceRole() override;

    auto scene_surface() const -> std::optional<std::shared_ptr<scene::Surface>> override;

    void populate_spec_with_surface_data(shell::SurfaceSpecification& spec);
    void refresh_surface_data_now() override;

    void apply_spec(shell::SurfaceSpecification const& new_spec);
    void set_pending_offset(std::optional<geometry::Displacement> const& offset);
    void set_pending_width(std::optional<geometry::Width> const& width);
    void set_pending_height(std::optional<geometry::Height> const& height);
    void set_title(std::string const& title);
    void set_application_id(std::string const& application_id);
    void initiate_interactive_move(uint32_t serial);
    void initiate_interactive_resize(MirResizeEdge edge, uint32_t serial);
    void set_parent(std::optional<std::shared_ptr<scene::Surface>> const& parent);
    void set_max_size(int32_t width, int32_t height);
    void set_min_size(int32_t width, int32_t height);
    void set_fullscreen(std::optional<wl_resource*> const& output);

    void set_type(MirWindowType type);

    void add_state_now(MirWindowState state);
    void remove_state_now(MirWindowState state);
    void create_scene_surface();
    void track_overlapping_outputs();

    /// Gets called after the surface has committed (so current_size() may return the committed buffer size) but before
    /// the Mir window is modified (so if a pending size is set or a spec is applied those changes will take effect)
    virtual void handle_commit() = 0;

    virtual void handle_state_change(MirWindowState new_state) = 0;
    virtual void handle_active_change(bool is_now_active) = 0;
    virtual void handle_resize(
        std::optional<geometry::Point> const& new_top_left,
        geometry::Size const& new_size) = 0;
    virtual void handle_close_request() = 0;

protected:
    /// The size the window will be after the next commit
    auto pending_size() const -> geometry::Size;

    /// The size the window currently is (the committed size, or a reasonable default if it has never committed)
    auto current_size() const -> geometry::Size;

    /// Window size requested by Mir
    auto requested_window_size() const -> std::optional<geometry::Size>;

    auto window_state() const -> MirWindowState;
    auto is_active() const -> bool;

    void commit(WlSurfaceState const& state) override;
    void surface_destroyed() override;

    auto input_event_for(uint32_t serial) -> std::shared_ptr<MirInputEvent const>;

private:
    wayland::Weak<WlSurface> const surface;
    wayland::Client* const client;
    std::shared_ptr<shell::Shell> const shell;
    std::shared_ptr<scene::Session> const session;
    OutputManager* output_manager;
    Executor& wayland_executor;
    std::shared_ptr<WaylandSurfaceObserver> const observer;
    bool scene_surface_marked_ready{false};
    std::weak_ptr<scene::Surface> weak_scene_surface;

    /// The explicitly set (not taken from the surface buffer size) uncommitted window size
    /// @{
    std::optional<geometry::Width> pending_explicit_width;
    std::optional<geometry::Height> pending_explicit_height;
    /// @}

    /// If the committed window size was set explicitly, rather than being taken from the buffer size
    /// @{
    bool committed_width_set_explicitly{false};
    bool committed_height_set_explicitly{false};
    /// @}

    /// The min and max size of the window as of last commit
    /// @{
    geometry::Size committed_min_size;
    geometry::Size committed_max_size;
    /// @}

    std::unique_ptr<shell::SurfaceSpecification> pending_changes;

    struct TrackedOutput
    {
        mir::graphics::DisplayConfigurationOutput config;
        bool bound{true};
    };
    std::vector<TrackedOutput> tracked_outputs;

    shell::SurfaceSpecification& spec();

    // Ask the derived class to destroy the wayland role object (as only it can do that)
    virtual void destroy_role() const = 0;

    void apply_client_size(mir::shell::SurfaceSpecification& mods);
};

}
}

#endif // MIR_FRONTEND_WINDOW_WL_SURFACE_ROLE_H
