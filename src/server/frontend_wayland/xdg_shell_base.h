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

#ifndef MIR_FRONTEND_XDG_SHELL_BASE_H
#define MIR_FRONTEND_XDG_SHELL_BASE_H

#include "wl_mir_window.h"

#include "mir/geometry/size.h"
#include "mir/geometry/rectangle.h"

#include "mir_toolkit/common.h"

#include <cstdint>
#include <memory>
#include <experimental/optional>

struct wl_display;
struct wl_resource;

namespace mir
{
namespace frontend
{

class Shell;
class XdgSurfaceBase;
class XdgPositionerBase;
class WlSeat;
class XdgSurfaceBaseEventSink;

// a base class for the shell itself would not be useful

class XdgSurfaceBase : WlAbstractMirWindow
{
public:
    class AdapterInterface
    {
    public:
        virtual wl_resource* get_resource() const = 0;
        virtual void create_toplevel(uint32_t id) const = 0; // new XdgToplevel{client, resource, id};
        virtual void create_popup(uint32_t id) const = 0; // new XdgPopup{client, resource, id};
        virtual ~AdapterInterface() = default;
    };

    XdgSurfaceBase(AdapterInterface const& adapter, wl_client* client, wl_resource* resource, wl_resource* parent,
                   wl_resource* surface, std::shared_ptr<Shell> const& shell, WlSeat& seat);
    ~XdgSurfaceBase() override;

    void destroy() override; // overrides from WlMirWindow but can also be called from destroy request

    void become_toplevel(uint32_t id);
    void become_popup(uint32_t id, std::experimental::optional<XdgSurfaceBase const* const> const& parent,
                      XdgPositionerBase const& positioner);
    void set_window_geometry(int32_t x, int32_t y, int32_t width, int32_t height);
    void ack_configure(uint32_t serial);

    void set_parent(optional_value<SurfaceId> parent_id);
    void set_title(std::string const& title);
    void move();
    void resize(MirResizeEdge edge);
    void set_notify_resize(std::function<void(geometry::Size const& new_size)> notify_resize);
    void set_max_size(int32_t width, int32_t height);
    void set_min_size(int32_t width, int32_t height);
    void set_maximized();
    void unset_maximized();

    using WlAbstractMirWindow::client;
    using WlAbstractMirWindow::params;
    using WlAbstractMirWindow::surface_id;

    struct wl_resource* const parent;
    std::shared_ptr<Shell> const shell;
    std::shared_ptr<XdgSurfaceBaseEventSink> const sink;
    AdapterInterface const& adapter;
};

struct XdgPositionerBase
{
    optional_value<geometry::Size> size;
    optional_value<geometry::Rectangle> aux_rect;
    optional_value<MirPlacementGravity> surface_placement_gravity;
    optional_value<MirPlacementGravity> aux_rect_placement_gravity;
    optional_value<int> aux_rect_placement_offset_x;
    optional_value<int> aux_rect_placement_offset_y;
};

}
}

#endif // MIR_FRONTEND_XDG_SHELL_BASE_H
