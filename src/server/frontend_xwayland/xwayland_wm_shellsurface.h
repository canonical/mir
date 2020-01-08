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

#include "wayland_wrapper.h"
#include "wl_surface.h"
#include "window_wl_surface_role.h"
#include "xwayland_wm_surface.h"

namespace mir
{
namespace frontend
{
class OutputManager;
class XWaylandWMSurface;
class XWaylandWMShellSurface : public WindowWlSurfaceRole
{
public:
    XWaylandWMShellSurface(
        XWaylandWMSurface* xwayland_surface,
        wl_client* client,
        WlSurface* wayland_surface,
        std::shared_ptr<shell::Shell> const& shell,
        WlSeat& seat,
        OutputManager* const output_manager);
    ~XWaylandWMShellSurface();

    void set_toplevel();
    void resize(uint32_t edges);

protected:
    void destroy() override;
    void handle_commit() override {};
    void handle_state_change(MirWindowState /*new_state*/) override {};
    void handle_active_change(bool /*is_now_active*/) override {};
    void handle_resize(std::experimental::optional<geometry::Point> const& new_top_left, geometry::Size const& new_size) override;
    void handle_close_request() override;

private:
    XWaylandWMSurface* const surface;
};
} /* frontend */
} /* mir */

#endif /* end of include guard: MIR_FRONTEND_XWAYLAND_WM_SHELLSURFACE_ROLE_H */
