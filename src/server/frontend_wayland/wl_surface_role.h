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

#ifndef MIR_FRONTEND_WL_SURFACE_ROLE_H
#define MIR_FRONTEND_WL_SURFACE_ROLE_H

#include "mir/frontend/surface_id.h"
#include "mir/geometry/displacement.h"
#include "mir/geometry/size.h"
#include "mir/optional_value.h"

#include <mir_toolkit/common.h>

#include <experimental/optional>
#include <memory>

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
class BasicSurfaceEventSink;
class OutputManager;
class WlSurface;
class WlSeat;
struct WlSurfaceState;

class WlSurfaceRole
{
public:
    virtual bool synchronized() const { return false; }
    virtual geometry::Displacement total_offset() const { return {}; }
    virtual SurfaceId surface_id() const = 0;
    virtual void refresh_surface_data_now() = 0;
    virtual void commit(WlSurfaceState const& state) = 0;
    virtual void visiblity(bool visible) = 0;
    virtual void destroy() = 0;
    virtual ~WlSurfaceRole() = default;
};

class WlAbstractMirWindow : public WlSurfaceRole
{
public:
    WlAbstractMirWindow(WlSeat* seat, wl_client* client, WlSurface* surface,
                        std::shared_ptr<frontend::Shell> const& shell, OutputManager* output_manager);

    ~WlAbstractMirWindow() override;

    SurfaceId surface_id() const override { return surface_id_; };

    void populate_spec_with_surface_data(shell::SurfaceSpecification& spec);
    void refresh_surface_data_now() override;

    void set_maximized();
    void unset_maximized();
    void set_fullscreen(std::experimental::optional<wl_resource*> const& output);
    void unset_fullscreen();
    void set_minimized();

    void set_state_now(MirWindowState state);

    virtual void handle_resize(geometry::Size const& new_size) = 0;

protected:
    std::shared_ptr<bool> const destroyed;
    wl_client* const client;
    WlSurface* const surface;
    std::shared_ptr<frontend::Shell> const shell;
    OutputManager* output_manager;
    std::shared_ptr<BasicSurfaceEventSink> const sink;

    std::unique_ptr<scene::SurfaceCreationParameters> const params;
    optional_value<geometry::Size> window_size_;

    geometry::Size window_size();
    shell::SurfaceSpecification& spec();
    void commit(WlSurfaceState const& state) override;

private:
    SurfaceId surface_id_;
    std::unique_ptr<shell::SurfaceSpecification> pending_changes;

    void visiblity(bool visible) override;

    void create_mir_window();
};

}
}

#endif // MIR_FRONTEND_WL_SURFACE_ROLE_H
