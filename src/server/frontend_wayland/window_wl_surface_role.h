/*
 * Copyright © 2015-2018 Canonical Ltd.
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
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#ifndef MIR_FRONTEND_WINDOW_WL_SURFACE_ROLE_H
#define MIR_FRONTEND_WINDOW_WL_SURFACE_ROLE_H

#include "wl_surface_role.h"

#include "mir/frontend/surface_id.h"
#include "mir/geometry/displacement.h"
#include "mir/geometry/size.h"
#include "mir/geometry/rectangle.h"

#include <mir_toolkit/common.h>

#include <experimental/optional>
#include <chrono>

struct wl_client;
struct wl_resource;

namespace mir
{
namespace scene
{
struct SurfaceCreationParameters;
class Surface;
class Session;
}
namespace shell
{
struct SurfaceSpecification;
class Shell;
}
namespace frontend
{
class WaylandSurfaceObserver;
class OutputManager;
class WlSurface;
class WlSeat;

class WindowWlSurfaceRole : public WlSurfaceRole
{
public:

    WindowWlSurfaceRole(
        WlSeat* seat,
        wl_client* client,
        WlSurface* surface,
        std::shared_ptr<shell::Shell> const& shell,
        OutputManager* output_manager);

    ~WindowWlSurfaceRole() override;

    std::shared_ptr<bool> destroyed_flag() const { return destroyed; }
    auto scene_surface() const -> std::experimental::optional<std::shared_ptr<scene::Surface>> override;

    void populate_spec_with_surface_data(shell::SurfaceSpecification& spec);
    void refresh_surface_data_now() override;

    void apply_spec(shell::SurfaceSpecification const& new_spec);
    void set_pending_offset(std::experimental::optional<geometry::Displacement> const& offset);
    void set_pending_width(std::experimental::optional<geometry::Width> const& width);
    void set_pending_height(std::experimental::optional<geometry::Height> const& height);
    void set_title(std::string const& title);
    void set_application_id(std::string const& application_id);
    void initiate_interactive_move();
    void initiate_interactive_resize(MirResizeEdge edge);
    void set_parent(std::experimental::optional<std::shared_ptr<scene::Surface>> const& parent);
    void set_max_size(int32_t width, int32_t height);
    void set_min_size(int32_t width, int32_t height);
    void set_fullscreen(std::experimental::optional<wl_resource*> const& output);
    void set_server_side_decorated(bool server_side_decorated);

    void set_state_now(MirWindowState state);
    void create_mir_window();

    /// Gets called after the surface has committed (so current_size() may return the committed buffer size) but before
    /// the Mir window is modified (so if a pending size is set or a spec is applied those changes will take effect)
    virtual void handle_commit() = 0;

    virtual void handle_state_change(MirWindowState new_state) = 0;
    virtual void handle_active_change(bool is_now_active) = 0;
    virtual void handle_resize(std::experimental::optional<geometry::Point> const& new_top_left,
                               geometry::Size const& new_size) = 0;
    virtual void handle_close_request() = 0;

protected:
    std::shared_ptr<bool> const destroyed;

    /// The size the window will be after the next commit
    auto pending_size() const -> geometry::Size;

    /// The size the window currently is (the committed size, or a reasonable default if it has never committed)
    auto current_size() const -> geometry::Size;

    /// Window size requested by Mir
    std::experimental::optional<geometry::Size> requested_window_size();

    auto window_state() -> MirWindowState;
    auto is_active() -> bool;
    auto latest_timestamp() -> std::chrono::nanoseconds;

    void commit(WlSurfaceState const& state) override;

private:
    WlSurface* const surface;
    wl_client* client;
    std::shared_ptr<shell::Shell> const shell;
    std::shared_ptr<scene::Session> const session;
    OutputManager* output_manager;
    std::shared_ptr<WaylandSurfaceObserver> const observer;
    std::unique_ptr<scene::SurfaceCreationParameters> const params;
    std::weak_ptr<scene::Surface> weak_scene_surface;

    /// The explicitly set (not taken from the surface buffer size) uncommitted window size
    /// @{
    std::experimental::optional<geometry::Width> pending_explicit_width;
    std::experimental::optional<geometry::Height> pending_explicit_height;
    /// @}

    /// If the committed window size was set explicitly, rather than being taken from the buffer size
    /// @{
    bool committed_width_set_explicitly{false};
    bool committed_height_set_explicitly{false};
    /// @}

    /// The last committed window size (either explicitly set or taken from the surface buffer size)
    std::experimental::optional<geometry::Size> committed_size;

    /// The min and max size of the window as of last commit
    /// @{
    geometry::Size committed_min_size;
    geometry::Size committed_max_size;
    /// @}

    std::unique_ptr<shell::SurfaceSpecification> pending_changes;

    void visiblity(bool visible) override;

    shell::SurfaceSpecification& spec();
};

}
}

#endif // MIR_FRONTEND_WINDOW_WL_SURFACE_ROLE_H
