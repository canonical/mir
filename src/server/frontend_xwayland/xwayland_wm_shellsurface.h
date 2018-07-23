/*
 * Copyright (C) 2018 Marius Gripsgard <marius@ubports.com>
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
 *
 */

#ifndef MIR_FRONTEND_XWAYLAND_WM_SHELLSURFACE_ROLE_H
#define MIR_FRONTEND_XWAYLAND_WM_SHELLSURFACE_ROLE_H

#include "generated/wayland_wrapper.h"
#include "wl_surface.h"
#include "window_wl_surface_role.h"
#include "xwayland_wm_surface.h"

namespace mir
{
namespace frontend
{
class OutputManager;
class XWaylandWMSurface;
class Shell;
class XWaylandWMShellSurface : public WindowWlSurfaceRole
{
public:
    XWaylandWMShellSurface(wl_client* client, WlSurface* surface,
                           std::shared_ptr<Shell> const& shell, WlSeat& seat,
                           OutputManager* const output_manager);
    ~XWaylandWMShellSurface();

    void move();
    void set_fullscreen();
    void set_popup(struct wl_resource* /*seat*/,
                   uint32_t /*serial*/,
                   struct wl_resource* parent,
                   int32_t x,
                   int32_t y,
                   uint32_t flags);
    void set_title(std::string const& title);
    void resize(uint32_t edges);
    void set_toplevel();
    void set_surface(XWaylandWMSurface* sur);

    using WindowWlSurfaceRole::set_state_now;

protected:
    void destroy() override;
    void set_transient(struct wl_resource* parent, int32_t x, int32_t y, uint32_t flags);
    void handle_resize(const geometry::Size& new_size) override;

    using WindowWlSurfaceRole::client;
    using WindowWlSurfaceRole::surface_id;

private:
    XWaylandWMSurface *surface;
};
} /* frontend */
} /* mir */

#endif /* end of include guard: MIR_FRONTEND_XWAYLAND_WM_SHELLSURFACE_ROLE_H */
