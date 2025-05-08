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

#include "wl_shell.h"

#include "wl_surface.h"
#include "window_wl_surface_role.h"
#include "mir/shell/surface_specification.h"
#include "mir/frontend/surface.h"
#include "mir/log.h"

namespace mf = mir::frontend;
namespace mw = mir::wayland;
namespace ms = mir::scene;
namespace msh = mir::shell;
namespace geom = mir::geometry;

namespace mir
{
namespace frontend
{
class WlShellSurface : public wayland::ShellSurface, public WindowWlSurfaceRole
{
public:
    WlShellSurface(
        wl_resource* new_resource,
        Executor& wayland_executor,
        WlSurface* surface,
        std::shared_ptr<msh::Shell> const& shell,
        WlSeat& seat,
        OutputManager* output_manager)
        : ShellSurface(new_resource, Version<1>()),
          WindowWlSurfaceRole{wayland_executor, &seat, wayland::ShellSurface::client, surface, shell, output_manager}
    {
    }

    ~WlShellSurface() = default;

protected:
    void set_toplevel() override
    {
    }

    void set_transient(
        struct wl_resource* parent,
        int32_t x,
        int32_t y,
        uint32_t flags) override
    {
        auto& parent_surface = *WlSurface::from(parent);

        mir::shell::SurfaceSpecification mods;

        if (flags & mw::ShellSurface::Transient::inactive)
            mods.type = mir_window_type_tip;
        if (auto parent = parent_surface.scene_surface())
            mods.parent = parent.value();
        mods.aux_rect = geom::Rectangle{{x, y}, {}};
        mods.surface_placement_gravity = mir_placement_gravity_northwest;
        mods.aux_rect_placement_gravity = mir_placement_gravity_southeast;
        mods.placement_hints = mir_placement_hints_slide_x;
        mods.aux_rect_placement_offset_x = 0;
        mods.aux_rect_placement_offset_y = 0;

        apply_spec(mods);
    }

    void handle_commit() override {};
    void handle_state_change(MirWindowState /*new_state*/) override {};
    void handle_active_change(bool /*is_now_active*/) override {};

    void handle_resize(
        std::optional<geometry::Point> const& /*new_top_left*/,
        geometry::Size const& new_size) override
    {
        send_configure_event(
            mw::ShellSurface::Resize::none,
            new_size.width.as_int(),
            new_size.height.as_int());
    }

    void handle_close_request() override
    {
        // It seems there is no way to request close of a wl_shell_surface
    }

    void handle_tiled_edges(Flags<MirTiledEdge> /*tiled_edges*/) override {}

    void set_fullscreen(
        uint32_t /*method*/,
        uint32_t /*framerate*/,
        std::optional<struct wl_resource*> const& output) override
    {
        WindowWlSurfaceRole::set_fullscreen(output);
    }

    void set_popup(
        struct wl_resource* /*seat*/,
        uint32_t /*serial*/,
        struct wl_resource* parent,
        int32_t x,
        int32_t y,
        uint32_t flags) override
    {
        auto& parent_surface = *WlSurface::from(parent);

        mir::shell::SurfaceSpecification mods;

        if (flags & mw::ShellSurface::Transient::inactive)
            mods.type = mir_window_type_tip;
        if (auto parent = parent_surface.scene_surface())
            mods.parent = parent.value();
        mods.aux_rect = geom::Rectangle{{x, y}, {}};
        mods.surface_placement_gravity = mir_placement_gravity_northwest;
        mods.aux_rect_placement_gravity = mir_placement_gravity_southeast;
        mods.placement_hints = mir_placement_hints_slide_x;
        mods.aux_rect_placement_offset_x = 0;
        mods.aux_rect_placement_offset_y = 0;

        apply_spec(mods);
    }

    void set_maximized(std::optional<struct wl_resource*> const& output) override
    {
        (void)output;
        WindowWlSurfaceRole::add_state_now(mir_window_state_maximized);
    }

    void set_title(std::string const& title) override
    {
        WindowWlSurfaceRole::set_title(title);
    }

    void pong(uint32_t /*serial*/) override
    {
    }

    void move(struct wl_resource* /*seat*/, uint32_t serial) override
    {
        WindowWlSurfaceRole::initiate_interactive_move(serial);
    }

    void resize(struct wl_resource* /*seat*/, uint32_t serial, uint32_t edges) override
    {
        MirResizeEdge edge = mir_resize_edge_none;

        switch (edges)
        {
        case mw::ShellSurface::Resize::top:
            edge = mir_resize_edge_north;
            break;

        case mw::ShellSurface::Resize::bottom:
            edge = mir_resize_edge_south;
            break;

        case mw::ShellSurface::Resize::left:
            edge = mir_resize_edge_west;
            break;

        case mw::ShellSurface::Resize::top_left:
            edge = mir_resize_edge_northwest;
            break;

        case mw::ShellSurface::Resize::bottom_left:
            edge = mir_resize_edge_southwest;
            break;

        case mw::ShellSurface::Resize::right:
            edge = mir_resize_edge_east;
            break;

        case mw::ShellSurface::Resize::top_right:
            edge = mir_resize_edge_northeast;
            break;

        case mw::ShellSurface::Resize::bottom_right:
            edge = mir_resize_edge_southeast;
            break;

        default:;
        }

        WindowWlSurfaceRole::initiate_interactive_resize(edge, serial);
    }

    void set_class(std::string const& /*class_*/) override
    {
    }

    void destroy_role() const override
    {
        wl_resource_destroy(resource);
    }
};

class WlShell : public wayland::Shell::Global
{
public:
    WlShell(
        wl_display* display,
        Executor& wayland_executor,
        std::shared_ptr<msh::Shell> const& shell,
        WlSeat& seat,
        OutputManager* const output_manager)
        : Global(display, Version<1>()),
          wayland_executor{wayland_executor},
          shell{shell},
          seat{seat},
          output_manager{output_manager}
    {
    }

    static auto get_window(wl_resource* window) -> std::shared_ptr<Surface>;

private:
    Executor& wayland_executor;
    std::shared_ptr<msh::Shell> const shell;
    WlSeat& seat;
    OutputManager* const output_manager;

    class Instance : public wayland::Shell
    {
    public:
        Instance(wl_resource* new_resource, WlShell* shell)
            : mw::Shell{new_resource, Version<1>()},
              shell{shell}
        {
        }

    private:
        WlShell* const shell;

        void get_shell_surface(wl_resource* new_shell_surface, wl_resource* surface) override
        {
            new WlShellSurface(
                new_shell_surface,
                shell->wayland_executor,
                WlSurface::from(surface),
                shell->shell,
                shell->seat,
                shell->output_manager);
        }
    };

    void bind(wl_resource* new_resource) override
    {
        new Instance{new_resource, this};
    }
};
}
}

auto mf::create_wl_shell(
    wl_display* display,
    Executor& wayland_executor,
    std::shared_ptr<msh::Shell> const& shell,
    WlSeat* seat,
    OutputManager* const output_manager)
-> std::shared_ptr<mw::Shell::Global>
{
    return std::make_shared<mf::WlShell>(display, wayland_executor, shell, *seat, output_manager);
}

auto mf::get_wl_shell_window(wl_resource* surface) -> std::shared_ptr<ms::Surface>
{
    if (mw::Surface::is_instance(surface))
    {
        auto const wlsurface = WlSurface::from(surface);
        if (auto const scene_surface = wlsurface->scene_surface())
        {
            return scene_surface.value();
        }

        log_debug("No window currently associated with wayland::Surface %p", static_cast<void*>(surface));
    }

    return {};
}
