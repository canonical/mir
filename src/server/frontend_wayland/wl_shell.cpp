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
#include "wl_seat.h"
#include "window_wl_surface_role.h"
#include "weak.h"
#include <mir/shell/surface_specification.h>
#include <mir/frontend/surface.h>
#include <mir/log.h>

namespace mf = mir::frontend;
namespace mwrs = mir::wayland_rs;
namespace ms = mir::scene;
namespace msh = mir::shell;
namespace geom = mir::geometry;

namespace mir
{
namespace frontend
{
class WlShellSurface : public wayland_rs::ShellSurface, public WindowWlSurfaceRole
{
public:
    using wayland_rs::ShellSurface::destroyed_flag;

    WlShellSurface(
        std::shared_ptr<wayland_rs::Client> client,
        rust::Box<wayland_rs::ShellSurfaceMiddleware> instance,
        uint32_t object_id,
        Executor& wayland_executor,
        WlSurface* surface,
        std::shared_ptr<msh::Shell> const& shell,
        WlSeat& seat,
        OutputManager* output_manager,
        std::shared_ptr<SurfaceRegistry> const& surface_registry) :
        ShellSurface{std::move(client), std::move(instance), object_id},
        WindowWlSurfaceRole{
            wayland_executor,
            &seat,
            wayland_rs::ShellSurface::client,
            surface,
            shell,
            output_manager,
            surface_registry}
    {
    }

    ~WlShellSurface() = default;

protected:
    void set_toplevel() override
    {
    }

    using wayland_rs::ShellSurface::set_transient;
    auto set_transient(
        wayland_rs::Weak<wayland_rs::Surface> const& parent,
        int32_t x,
        int32_t y,
        uint32_t flags) -> void override
    {
        auto& parent_surface = *wayland_rs::Surface::from<WlSurface>(parent);

        mir::shell::SurfaceSpecification mods;

        if (flags & Transient::inactive)
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
            Resize::none,
            new_size.width.as_int(),
            new_size.height.as_int());
    }

    void handle_close_request() override
    {
        // It seems there is no way to request close of a wl_shell_surface
    }

    void handle_tiled_edges(Flags<MirTiledEdge> /*tiled_edges*/) override {}

    using wayland_rs::ShellSurface::set_fullscreen;
    auto set_fullscreen(
        uint32_t /*method*/,
        uint32_t /*framerate*/,
        std::optional<wayland_rs::Weak<wayland_rs::Output>> const& output) -> void override
    {
        WindowWlSurfaceRole::set_fullscreen(output);
    }

    using wayland_rs::ShellSurface::set_popup;
    auto set_popup(
        wayland_rs::Weak<wayland_rs::Seat> const& /*seat*/,
        uint32_t /*serial*/,
        wayland_rs::Weak<wayland_rs::Surface> const& parent,
        int32_t x,
        int32_t y,
        uint32_t flags) -> void override
    {
        auto& parent_surface = *wayland_rs::Surface::from<WlSurface>(parent);

        mir::shell::SurfaceSpecification mods;

        if (flags & Transient::inactive)
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

    using wayland_rs::ShellSurface::set_maximized;
    auto set_maximized(std::optional<wayland_rs::Weak<wayland_rs::Output>> const& output) -> void override
    {
        (void)output;
        WindowWlSurfaceRole::add_state_now(mir_window_state_maximized);
    }

    auto set_title(rust::String title) -> void override
    {
        WindowWlSurfaceRole::set_title(std::string{title});
    }

    auto pong(uint32_t /*serial*/) -> void override
    {
    }

    using wayland_rs::ShellSurface::r_move;
    auto r_move(wayland_rs::Weak<wayland_rs::Seat> const& /*seat*/, uint32_t serial) -> void override
    {
        WindowWlSurfaceRole::initiate_interactive_move(serial);
    }

    using wayland_rs::ShellSurface::resize;
    auto resize(wayland_rs::Weak<wayland_rs::Seat> const& /*seat*/, uint32_t serial, uint32_t edges) -> void override
    {
        MirResizeEdge edge = mir_resize_edge_none;

        switch (edges)
        {
        case Resize::top:
            edge = mir_resize_edge_north;
            break;

        case Resize::bottom:
            edge = mir_resize_edge_south;
            break;

        case Resize::left:
            edge = mir_resize_edge_west;
            break;

        case Resize::top_left:
            edge = mir_resize_edge_northwest;
            break;

        case Resize::bottom_left:
            edge = mir_resize_edge_southwest;
            break;

        case Resize::right:
            edge = mir_resize_edge_east;
            break;

        case Resize::top_right:
            edge = mir_resize_edge_northeast;
            break;

        case Resize::bottom_right:
            edge = mir_resize_edge_southeast;
            break;

        default:;
        }

        WindowWlSurfaceRole::initiate_interactive_resize(edge, serial);
    }

    auto set_class(rust::String /*class_*/) -> void override
    {
    }

    void destroy_role() const override
    {
        const_cast<WlShellSurface*>(this)->destroy_and_delete();
    }
};

class WlShell : public wayland_rs::Shell
{
public:
    WlShell(
        std::shared_ptr<wayland_rs::Client> client,
        rust::Box<wayland_rs::ShellMiddleware> instance,
        uint32_t object_id,
        Executor& wayland_executor,
        std::shared_ptr<msh::Shell> const& shell,
        WlSeat& seat,
        OutputManager* const output_manager,
        std::shared_ptr<SurfaceRegistry> const& surface_registry)
        : Shell{std::move(client), std::move(instance), object_id},
          wayland_executor{wayland_executor},
          shell{shell},
          seat{seat},
          output_manager{output_manager},
          surface_registry{surface_registry}
    {
    }

private:
    Executor& wayland_executor;
    std::shared_ptr<msh::Shell> const shell;
    WlSeat& seat;
    OutputManager* const output_manager;
    std::shared_ptr<SurfaceRegistry> const surface_registry;

    using wayland_rs::Shell::get_shell_surface;
    auto get_shell_surface(
        wayland_rs::Weak<wayland_rs::Surface> const& surface,
        rust::Box<wayland_rs::ShellSurfaceMiddleware> child_instance,
        uint32_t child_object_id) -> std::shared_ptr<wayland_rs::ShellSurface> override
    {
        return std::make_shared<WlShellSurface>(
            client,
            std::move(child_instance),
            child_object_id,
            wayland_executor,
            wayland_rs::Surface::from<WlSurface>(surface),
            shell,
            seat,
            output_manager,
            surface_registry);
    }
};
}
}

auto mf::create_wl_shell(
    std::shared_ptr<wayland_rs::Client> client,
    rust::Box<wayland_rs::ShellMiddleware> instance,
    uint32_t object_id,
    Executor& wayland_executor,
    std::shared_ptr<msh::Shell> const& shell,
    WlSeat* seat,
    OutputManager* const output_manager,
    std::shared_ptr<SurfaceRegistry> const& surface_registry)
-> std::shared_ptr<mwrs::Shell>
{
    return std::make_shared<mf::WlShell>(
        std::move(client),
        std::move(instance),
        object_id,
        wayland_executor,
        shell,
        *seat,
        output_manager,
        surface_registry);
}

auto mf::get_wl_shell_window(
    mwrs::Weak<mwrs::ShellSurface> const& surface) -> std::shared_ptr<ms::Surface>
{
    if (auto const shell_surface = mwrs::ShellSurface::from<WlShellSurface>(surface))
    {
        if (auto const scene_surface = shell_surface->scene_surface())
        {
            return scene_surface.value();
        }

        log_debug("No window currently associated with wayland_rs::ShellSurface@%u", shell_surface->object_id());
    }

    return {};
}
