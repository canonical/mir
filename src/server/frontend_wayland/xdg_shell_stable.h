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

#ifndef MIR_FRONTEND_XDG_SHELL_STABLE_H
#define MIR_FRONTEND_XDG_SHELL_STABLE_H

#include "xdg_shell.h"
#include "window_wl_surface_role.h"

#include <memory>
#include <optional>

namespace mir
{
namespace scene
{
class Surface;
}
namespace shell
{
class Shell;
}
namespace frontend
{
class WlSeat;
class OutputManager;
class WlSurface;
class XdgSurfaceStable;
class XdgPositionerStable;

class XdgShellStable : public wayland_rs::XdgWmBase
{
public:
    XdgShellStable(
        std::shared_ptr<wayland_rs::Client> client,
        rust::Box<wayland_rs::XdgWmBaseMiddleware> instance,
        uint32_t object_id,
        Executor& wayland_executor,
        std::shared_ptr<shell::Shell> const& shell,
        WlSeat& seat,
        OutputManager* output_manager,
        std::shared_ptr<SurfaceRegistry> const& surface_registry);

    static auto get_window(
        wayland_rs::Weak<wayland_rs::XdgSurface> const& surface) -> std::shared_ptr<scene::Surface>;

    Executor& wayland_executor;
    std::shared_ptr<shell::Shell> const shell;
    WlSeat& seat;
    OutputManager* const output_manager;
    std::shared_ptr<SurfaceRegistry> const surface_registry;

private:
    auto create_positioner(
        rust::Box<wayland_rs::XdgPositionerMiddleware> child_instance,
        uint32_t child_object_id) -> std::shared_ptr<wayland_rs::XdgPositioner> override;

    using wayland_rs::XdgWmBase::get_xdg_surface;
    auto get_xdg_surface(
        wayland_rs::Weak<wayland_rs::Surface> const& surface,
        rust::Box<wayland_rs::XdgSurfaceMiddleware> child_instance,
        uint32_t child_object_id) -> std::shared_ptr<wayland_rs::XdgSurface> override;

    auto pong(uint32_t serial) -> void override;
};

auto create_xdg_shell_stable(
    std::shared_ptr<wayland_rs::Client> client,
    rust::Box<wayland_rs::XdgWmBaseMiddleware> instance,
    uint32_t object_id,
    Executor& wayland_executor,
    std::shared_ptr<shell::Shell> const& shell,
    WlSeat& seat,
    OutputManager* output_manager,
    std::shared_ptr<SurfaceRegistry> const& surface_registry)
-> std::shared_ptr<wayland_rs::XdgWmBase>;

class XdgPopupStable : public wayland_rs::XdgPopup, public WindowWlSurfaceRole
{
public:
    using wayland_rs::XdgPopup::destroyed_flag;

    XdgPopupStable(
        std::shared_ptr<wayland_rs::Client> client,
        rust::Box<wayland_rs::XdgPopupMiddleware> instance,
        uint32_t object_id,
        XdgSurfaceStable* xdg_surface,
        std::optional<WlSurfaceRole*> parent_role,
        XdgPositionerStable& positioner,
        WlSurface* surface);

    /// Used when the aux rect needs to be adjusted due to the parent logical Wayland surface not lining up with the
    /// parent scene surface (as is the case for layer shell surfaces with a margin)
    void set_aux_rect_offset_now(geometry::Displacement const& new_aux_rect_offset);

    using wayland_rs::XdgPopup::grab;
    auto grab(wayland_rs::Weak<wayland_rs::Seat> const& seat, uint32_t serial) -> void override;
    using wayland_rs::XdgPopup::reposition;
    auto reposition(wayland_rs::Weak<wayland_rs::XdgPositioner> const& positioner, uint32_t token) -> void override;

    void handle_commit() override {};
    void handle_state_change(MirWindowState /*new_state*/) override {};
    void handle_active_change(bool /*is_now_active*/) override {};
    void handle_resize(
        std::optional<geometry::Point> const& new_top_left,
        geometry::Size const& new_size) override;
    void handle_close_request() override;
    void handle_tiled_edges(Flags<MirTiledEdge> /*tiled_edges*/) override {}

    static auto from(wayland_rs::Weak<wayland_rs::XdgPopup> const& resource) -> XdgPopupStable*;

private:
    void destroy_role() const override;

    std::optional<geometry::Point> cached_top_left;
    std::optional<geometry::Size> cached_size;
    bool initial_configure_pending{true};
    std::optional<uint32_t> reposition_token;
    bool reactive;
    geometry::Rectangle aux_rect;
    geometry::Displacement aux_rect_offset;

    std::shared_ptr<shell::Shell> const shell;
    wayland_rs::Weak<XdgSurfaceStable> const xdg_surface;

    auto popup_rect() const -> std::optional<geometry::Rectangle>;
};

class XdgToplevelStable : public wayland_rs::XdgToplevel, public WindowWlSurfaceRole
{
public:
    using wayland_rs::XdgToplevel::destroyed_flag;

    XdgToplevelStable(
        std::shared_ptr<wayland_rs::Client> client,
        rust::Box<wayland_rs::XdgToplevelMiddleware> instance,
        uint32_t object_id,
        XdgSurfaceStable* xdg_surface,
        WlSurface* surface);

    using wayland_rs::XdgToplevel::set_parent;
    auto set_parent(std::optional<wayland_rs::Weak<wayland_rs::XdgToplevel>> const& parent) -> void override;
    auto set_title(rust::String title) -> void override;
    auto set_app_id(rust::String app_id) -> void override;
    using wayland_rs::XdgToplevel::show_window_menu;
    auto show_window_menu(
        wayland_rs::Weak<wayland_rs::Seat> const& seat,
        uint32_t serial,
        int32_t x,
        int32_t y) -> void override;
    using wayland_rs::XdgToplevel::r_move;
    auto r_move(wayland_rs::Weak<wayland_rs::Seat> const& seat, uint32_t serial) -> void override;
    using wayland_rs::XdgToplevel::resize;
    auto resize(wayland_rs::Weak<wayland_rs::Seat> const& seat, uint32_t serial, uint32_t edges) -> void override;
    auto set_max_size(int32_t width, int32_t height) -> void override;
    auto set_min_size(int32_t width, int32_t height) -> void override;
    auto set_maximized() -> void override;
    auto unset_maximized() -> void override;
    using wayland_rs::XdgToplevel::set_fullscreen;
    auto set_fullscreen(std::optional<wayland_rs::Weak<wayland_rs::Output>> const& output) -> void override;
    auto unset_fullscreen() -> void override;
    auto set_minimized() -> void override;

    void handle_commit() override;
    void handle_state_change(MirWindowState /*new_state*/) override;
    void handle_active_change(bool /*is_now_active*/) override;
    void handle_resize(std::optional<geometry::Point> const& new_top_left, geometry::Size const& new_size) override;
    void handle_close_request() override;
    void handle_tiled_edges(Flags<MirTiledEdge> tiled_edges) override;

    static auto from(wayland_rs::Weak<wayland_rs::XdgToplevel> const& surface) -> XdgToplevelStable*;

private:
    void send_toplevel_configure();
    void destroy_role() const override;

    wayland_rs::Weak<XdgSurfaceStable> const xdg_surface;
};
}
}

#endif // MIR_FRONTEND_XDG_SHELL_STABLE_H
