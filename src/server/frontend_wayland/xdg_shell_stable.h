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

#include "wayland_rs/wayland_rs_cpp/include/xdg_shell.h"
#include "window_wl_surface_role.h"
#include "client.h"

namespace mir
{
namespace scene
{
class Shell;
class Surface;
}
namespace frontend
{
class WlSeatGlobal;
class OutputManager;
class WlSurface;
class XdgSurfaceStable;
class XdgPositionerStable;

class XdgShellStable : public wayland_rs::XdgWmBaseImpl
{
public:
    XdgShellStable(
        std::shared_ptr<wayland_rs::Client> const& client,
        Executor& wayland_executor,
        std::shared_ptr<shell::Shell> const& shell,
        WlSeatGlobal& seat,
        OutputManager* output_manager,
        std::shared_ptr<SurfaceRegistry> const& surface_registry);

    static auto get_window(wayland_rs::XdgSurfaceImpl* surface) -> std::shared_ptr<scene::Surface>;
    auto create_positioner() -> std::shared_ptr<wayland_rs::XdgPositionerImpl> override;
    auto get_xdg_surface(wayland_rs::Weak<wayland_rs::WlSurfaceImpl> const& surface) -> std::shared_ptr<wayland_rs::XdgSurfaceImpl> override;
    auto pong(uint32_t serial) -> void override;

    std::shared_ptr<wayland_rs::Client> client;
    Executor& wayland_executor;
    std::shared_ptr<shell::Shell> const shell;
    WlSeatGlobal& seat;
    OutputManager* const output_manager;
    std::shared_ptr<SurfaceRegistry> const surface_registry;
};

class XdgPopupStable : public wayland_rs::XdgPopupImpl, public WindowWlSurfaceRole
{
public:
    XdgPopupStable(
        std::shared_ptr<wayland_rs::Client> const& client,
        XdgSurfaceStable* xdg_surface,
        std::optional<WlSurfaceRole*> parent_role,
        XdgPositionerStable& positioner,
        WlSurface* surface);

    /// Used when the aux rect needs to be adjusted due to the parent logical Wayland surface not lining up with the
    /// parent scene surface (as is the case for layer shell surfaces with a margin)
    void set_aux_rect_offset_now(geometry::Displacement const& new_aux_rect_offset);

    auto grab(wayland_rs::Weak<wayland_rs::WlSeatImpl> const& seat, uint32_t serial) -> void override;
    auto reposition(wayland_rs::Weak<wayland_rs::XdgPositionerImpl> const& positioner, uint32_t token) -> void override;

    void handle_commit() override {};
    void handle_state_change(MirWindowState /*new_state*/) override {};
    void handle_active_change(bool /*is_now_active*/) override {};
    void handle_resize(
        std::optional<geometry::Point> const& new_top_left,
        geometry::Size const& new_size) override;
    void handle_close_request() override;
    void handle_tiled_edges(Flags<MirTiledEdge> /*tiled_edges*/) override {}

    static auto from(wayland_rs::XdgPopupImpl* impl) -> XdgPopupStable*;

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

class XdgToplevelStable : public mir::wayland_rs::XdgToplevelImpl, public WindowWlSurfaceRole
{
public:
    XdgToplevelStable(
    std::shared_ptr<wayland_rs::Client> const& client,
        XdgSurfaceStable* xdg_surface,
        WlSurface* surface);

    auto set_parent(wayland_rs::Weak<XdgToplevelImpl> const& parent, bool has_parent) -> void override;
    auto set_title(rust::String title) -> void override;
    auto set_app_id(rust::String app_id) -> void override;
    auto show_window_menu(wayland_rs::Weak<wayland_rs::WlSeatImpl> const& seat, uint32_t serial, int32_t x, int32_t y) -> void override;
    auto r_move(wayland_rs::Weak<wayland_rs::WlSeatImpl> const& seat, uint32_t serial) -> void override;
    auto resize(wayland_rs::Weak<wayland_rs::WlSeatImpl> const& seat, uint32_t serial, uint32_t edges) -> void override;
    void set_max_size(int32_t width, int32_t height) override;
    void set_min_size(int32_t width, int32_t height) override;
    void set_maximized() override;
    void unset_maximized() override;
    auto set_fullscreen(wayland_rs::Weak<wayland_rs::WlOutputImpl> const& output, bool has_output) -> void override;
    void unset_fullscreen() override;
    void set_minimized() override;

    void handle_commit() override {};
    void handle_state_change(MirWindowState /*new_state*/) override;
    void handle_active_change(bool /*is_now_active*/) override;
    void handle_resize(std::optional<geometry::Point> const& new_top_left, geometry::Size const& new_size) override;
    void handle_close_request() override;
    void handle_tiled_edges(Flags<MirTiledEdge> tiled_edges) override;

    static XdgToplevelStable* from(wayland_rs::XdgToplevelImpl* impl);

private:
    void send_toplevel_configure();
    void destroy_role() const override;

    mir::wayland_rs::Weak<XdgSurfaceStable> const xdg_surface;
};
}
}

#endif // MIR_FRONTEND_XDG_SHELL_STABLE_H
