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

#include "xdg-shell_wrapper.h"
#include "window_wl_surface_role.h"

#include "mir/shell/surface_specification.h"

namespace mir
{
namespace scene
{
class Shell;
class Surface;
}
namespace frontend
{
class WlSeat;
class OutputManager;
class WlSurface;
class XdgSurfaceStable;
class XdgPositionerStable;

class XdgShellStable : public wayland::XdgWmBase::Global
{
public:
    XdgShellStable(
        wl_display* display,
        Executor& wayland_executor,
        std::shared_ptr<shell::Shell> shell,
        WlSeat& seat,
        OutputManager* output_manager);

    static auto get_window(wl_resource* surface) -> std::shared_ptr<scene::Surface>;

    Executor& wayland_executor;
    std::shared_ptr<shell::Shell> const shell;
    WlSeat& seat;
    OutputManager* const output_manager;

private:
    class Instance;
    void bind(wl_resource* new_resource) override;
};

class XdgPopupStable : public wayland::XdgPopup, public WindowWlSurfaceRole
{
public:
    XdgPopupStable(
        wl_resource* new_resource,
        XdgSurfaceStable* xdg_surface,
        std::optional<WlSurfaceRole*> parent_role,
        XdgPositionerStable& positioner,
        WlSurface* surface);

    /// Used when the aux rect needs to be adjusted due to the parent logical Wayland surface not lining up with the
    /// parent scene surface (as is the case for layer shell surfaces with a margin)
    void set_aux_rect_offset_now(geometry::Displacement const& new_aux_rect_offset);

    void grab(struct wl_resource* seat, uint32_t serial) override;
    void reposition(wl_resource* positioner, uint32_t token) override;

    void handle_commit() override {};
    void handle_state_change(MirWindowState /*new_state*/) override {};
    void handle_active_change(bool /*is_now_active*/) override {};
    void handle_resize(
        std::optional<geometry::Point> const& new_top_left,
        geometry::Size const& new_size) override;
    void handle_close_request() override;

    static auto from(wl_resource* resource) -> XdgPopupStable*;

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
    wayland::Weak<XdgSurfaceStable> const xdg_surface;

    auto popup_rect() const -> std::optional<geometry::Rectangle>;
};

class XdgPositionerStable : public wayland::XdgPositioner, public shell::SurfaceSpecification
{
public:
    XdgPositionerStable(wl_resource* new_resource);

    void ensure_complete() const;

    bool reactive{false};

private:
    void set_size(int32_t width, int32_t height) override;
    void set_anchor_rect(int32_t x, int32_t y, int32_t width, int32_t height) override;
    void set_anchor(uint32_t anchor) override;
    void set_gravity(uint32_t gravity) override;
    void set_constraint_adjustment(uint32_t constraint_adjustment) override;
    void set_offset(int32_t x, int32_t y) override;
    void set_reactive() override;
    void set_parent_size(int32_t parent_width, int32_t parent_height) override;
    void set_parent_configure(uint32_t serial) override;
};

}
}

#endif // MIR_FRONTEND_XDG_SHELL_STABLE_H
