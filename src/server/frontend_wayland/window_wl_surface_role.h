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
#include "mir/optional_value.h"

#include <mir_toolkit/common.h>

#include <experimental/optional>

struct wl_client;
struct wl_resource;

namespace mir
{
namespace scene
{
struct SurfaceCreationParameters;
}
namespace shell
{
struct SurfaceSpecification;
}
namespace frontend
{
class Shell;
class WlSurfaceEventSink;
class OutputManager;
class WlSurface;
class WlSeat;

class WindowWlSurfaceRole : public WlSurfaceRole
{
public:

    WindowWlSurfaceRole(WlSeat* seat, wl_client* client, WlSurface* surface,
                        std::shared_ptr<frontend::Shell> const& shell, OutputManager* output_manager);

    ~WindowWlSurfaceRole() override;

    std::shared_ptr<bool> destroyed_flag() const { return destroyed; }
    SurfaceId surface_id() const override { return surface_id_; };

    void populate_spec_with_surface_data(shell::SurfaceSpecification& spec);
    void refresh_surface_data_now() override;

    void apply_spec(shell::SurfaceSpecification const& new_spec);
    void set_geometry(int32_t x, int32_t y, int32_t width, int32_t height);
    void set_title(std::string const& title);
    void initiate_interactive_move();
    void initiate_interactive_resize(MirResizeEdge edge);
    void set_parent(optional_value<SurfaceId> parent_id);
    void set_max_size(int32_t width, int32_t height);
    void set_min_size(int32_t width, int32_t height);
    void set_fullscreen(std::experimental::optional<wl_resource*> const& output);

    void set_state_now(MirWindowState state);

    virtual void handle_resize(std::experimental::optional<geometry::Point> const& new_top_left,
                               geometry::Size const& new_size) = 0;

protected:
    std::shared_ptr<bool> const destroyed;

    geometry::Size window_size();
    MirWindowState window_state();
    bool is_active();
    uint64_t latest_timestamp_ns();

    void commit(WlSurfaceState const& state) override;

private:
    wl_client* const client;
    WlSurface* const surface;
    std::shared_ptr<frontend::Shell> const shell;
    OutputManager* output_manager;
    std::shared_ptr<WlSurfaceEventSink> const sink;
    std::unique_ptr<scene::SurfaceCreationParameters> const params;
    optional_value<geometry::Size> window_size_;
    SurfaceId surface_id_;
    std::unique_ptr<shell::SurfaceSpecification> pending_changes;

    void visiblity(bool visible) override;

    shell::SurfaceSpecification& spec();
    void create_mir_window();
};

}
}

#endif // MIR_FRONTEND_WINDOW_WL_SURFACE_ROLE_H
