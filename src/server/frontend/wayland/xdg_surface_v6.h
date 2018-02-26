/*
 * Copyright © 2018 Canonical Ltd.
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

#ifndef MIR_FRONTEND_XDG_SURFACE_V6_H
#define MIR_FRONTEND_XDG_SURFACE_V6_H

#include "xdg_shell_generated_interfaces.h"
#include "wl_mir_window.h"

#include "mir/geometry/size.h"
#include "mir/geometry/rectangle.h"

#include "mir/optional_value.h"
#include "mir_toolkit/common.h" // for MirPlacementGravity

namespace mir
{

namespace geometry
{
class Rectangle;
}

namespace frontend
{

class Shell;
class WlSeat;
class XdgSurfaceV6EventSink;

class XdgPositionerV6 : public wayland::XdgPositionerV6
{
public:
    XdgPositionerV6(struct wl_client* client, struct wl_resource* parent, uint32_t id);

    void destroy() override;
    void set_size(int32_t width, int32_t height) override;
    void set_anchor_rect(int32_t x, int32_t y, int32_t width, int32_t height) override;
    void set_anchor(uint32_t anchor) override;
    void set_gravity(uint32_t gravity) override;
    void set_constraint_adjustment(uint32_t constraint_adjustment) override;
    void set_offset(int32_t x, int32_t y) override;

    optional_value<geometry::Size> size;
    optional_value<geometry::Rectangle> aux_rect;
    optional_value<MirPlacementGravity> surface_placement_gravity;
    optional_value<MirPlacementGravity> aux_rect_placement_gravity;
    optional_value<int> aux_rect_placement_offset_x;
    optional_value<int> aux_rect_placement_offset_y;
};

struct XdgSurfaceV6 : wayland::XdgSurfaceV6, WlAbstractMirWindow
{
    XdgSurfaceV6* get_xdgsurface(wl_resource* surface) const;

    XdgSurfaceV6(wl_client* client, wl_resource* parent, uint32_t id, wl_resource* surface,
                 std::shared_ptr<Shell> const& shell, WlSeat& seat);
    ~XdgSurfaceV6() override;

    void destroy() override;
    void get_toplevel(uint32_t id) override;
    void get_popup(uint32_t id, struct wl_resource* parent, struct wl_resource* positioner) override;
    void set_window_geometry(int32_t x, int32_t y, int32_t width, int32_t height) override;
    void ack_configure(uint32_t serial) override;

    void set_parent(optional_value<SurfaceId> parent_id);
    void set_title(std::string const& title);
    void move(struct wl_resource* seat, uint32_t serial);
    void resize(struct wl_resource* /*seat*/, uint32_t /*serial*/, uint32_t edges);
    void set_max_size(int32_t width, int32_t height);
    void set_min_size(int32_t width, int32_t height);
    void set_maximized();
    void unset_maximized();

    using WlAbstractMirWindow::client;
    using WlAbstractMirWindow::params;
    using WlAbstractMirWindow::surface_id;
    struct wl_resource* const parent;
    std::shared_ptr<Shell> const shell;
    std::shared_ptr<XdgSurfaceV6EventSink> const sink;
};

}
}

#endif // MIR_FRONTEND_XDG_SURFACE_V6_H
