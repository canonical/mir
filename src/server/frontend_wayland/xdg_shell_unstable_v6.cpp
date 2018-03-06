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

#include "xdg_shell_unstable_v6.h"
#include "xdg_shell_base.h"

#include "wayland_utils.h"
#include "basic_surface_event_sink.h"
#include "wl_seat.h"
#include "wl_surface.h"
#include "wl_mir_window.h"

#include "mir/scene/surface_creation_parameters.h"
#include "mir/frontend/shell.h"
#include "mir/optional_value.h"

namespace mf = mir::frontend;
namespace geom = mir::geometry;

namespace mir
{
namespace frontend
{

class Shell;
class XdgSurfaceUnstableV6;
class WlSeat;
class XdgSurfaceUnstableV6EventSink;

class XdgSurfaceUnstableV6 : XdgSurfaceBase::AdapterInterface, wayland::XdgSurfaceV6
{
public:
    static XdgSurfaceUnstableV6* from(wl_resource* surface);

    XdgSurfaceUnstableV6(wl_client* client, wl_resource* parent, uint32_t id, wl_resource* surface,
                         std::shared_ptr<Shell> const& shell, WlSeat& seat);
    ~XdgSurfaceUnstableV6() override;

    // from wayland::XdgSurfaceV6
    void destroy() override;
    void get_toplevel(uint32_t id) override;
    void get_popup(uint32_t id, struct wl_resource* parent, struct wl_resource* positioner) override;
    void set_window_geometry(int32_t x, int32_t y, int32_t width, int32_t height) override;
    void ack_configure(uint32_t serial) override;

    // from XdgSurfaceBase::AdapterInterface
    wl_resource* get_resource() const override;
    void create_toplevel(uint32_t id) override;
    void create_popup(uint32_t id) override;

    std::shared_ptr<XdgSurfaceUnstableV6EventSink> sink;
    XdgSurfaceBase base;
};

class XdgPopupUnstableV6 : wayland::XdgPopupV6
{
public:
    XdgPopupUnstableV6(struct wl_client* client, struct wl_resource* parent, uint32_t id);

    void grab(struct wl_resource* seat, uint32_t serial) override;
    void destroy() override;
};

class XdgToplevelUnstableV6 : public wayland::XdgToplevelV6
{
public:
    XdgToplevelUnstableV6(struct wl_client* client, struct wl_resource* parent, uint32_t id,
                          std::shared_ptr<frontend::Shell> const& shell, XdgSurfaceUnstableV6* const xdg_surface);

    void destroy() override;
    void set_parent(std::experimental::optional<struct wl_resource*> const& parent) override;
    void set_title(std::string const& title) override;
    void set_app_id(std::string const& /*app_id*/) override;
    void show_window_menu(struct wl_resource* seat, uint32_t serial, int32_t x, int32_t y) override;
    void move(struct wl_resource* seat, uint32_t serial) override;
    void resize(struct wl_resource* seat, uint32_t serial, uint32_t edges) override;
    void set_max_size(int32_t width, int32_t height) override;
    void set_min_size(int32_t width, int32_t height) override;
    void set_maximized() override;
    void unset_maximized() override;
    void set_fullscreen(std::experimental::optional<struct wl_resource*> const& output) override;
    void unset_fullscreen() override;
    void set_minimized() override;

private:
    static XdgToplevelUnstableV6* from(wl_resource* surface);

    XdgSurfaceBase* const base;
};

class XdgPositionerUnstableV6 : public wayland::XdgPositionerV6
{
public:
    static XdgPositionerUnstableV6* from(wl_resource* resource);

    XdgPositionerUnstableV6(struct wl_client* client, struct wl_resource* parent, uint32_t id);

    void destroy() override;
    void set_size(int32_t width, int32_t height) override;
    void set_anchor_rect(int32_t x, int32_t y, int32_t width, int32_t height) override;
    void set_anchor(uint32_t anchor) override;
    void set_gravity(uint32_t gravity) override;
    void set_constraint_adjustment(uint32_t constraint_adjustment) override;
    void set_offset(int32_t x, int32_t y) override;

    XdgPositionerBase base;
};

class XdgSurfaceUnstableV6EventSink : public BasicSurfaceEventSink
{
public:
    using BasicSurfaceEventSink::BasicSurfaceEventSink;

    XdgSurfaceUnstableV6EventSink(WlSeat* seat, wl_client* client, wl_resource* target, wl_resource* xdg_surface_unstable_v6,
                                  std::shared_ptr<bool> const& destroyed)
        : BasicSurfaceEventSink(seat, client, target, destroyed),
          xdg_surface_unstable_v6{xdg_surface_unstable_v6}
    {
        run_unless_destroyed([this]()
            {
                uint32_t serial = wl_display_next_serial(wl_client_get_display(this->client));
                zxdg_surface_v6_send_configure(this->xdg_surface_unstable_v6, serial);
            });
    }

    void send_resize(geometry::Size const& new_size) const override
    {
        if (window_size != new_size)
        {
            run_unless_destroyed([this, new_size]()
                {
                    auto const serial = wl_display_next_serial(wl_client_get_display(client));
                    notify_resize(new_size);
                    zxdg_surface_v6_send_configure(xdg_surface_unstable_v6, serial);
                });
        }
    }

    std::function<void(geometry::Size const& new_size)> notify_resize = [](auto){};

private:
    wl_resource* xdg_surface_unstable_v6;
};

}
}

// XdgShellUnstableV6

mf::XdgShellUnstableV6::XdgShellUnstableV6(struct wl_display* display, std::shared_ptr<mf::Shell> const shell, WlSeat& seat)
    : wayland::XdgShellV6(display, 1),
      shell{shell},
      seat{seat}
{}

void mf::XdgShellUnstableV6::destroy(struct wl_client* client, struct wl_resource* resource)
{
    (void)client, (void)resource;
    // TODO
}

void mf::XdgShellUnstableV6::create_positioner(struct wl_client* client, struct wl_resource* resource, uint32_t id)
{
    new XdgPositionerUnstableV6{client, resource, id};
}

void mf::XdgShellUnstableV6::get_xdg_surface(struct wl_client* client, struct wl_resource* resource, uint32_t id,
                                     struct wl_resource* surface)
{
    new XdgSurfaceUnstableV6{client, resource, id, surface, shell, seat};
}

void mf::XdgShellUnstableV6::pong(struct wl_client* client, struct wl_resource* resource, uint32_t serial)
{
    (void)client, (void)resource, (void)serial;
    // TODO
}

// XdgSurfaceUnstableV6

mf::XdgSurfaceUnstableV6* mf::XdgSurfaceUnstableV6::from(wl_resource* surface)
{
    auto* tmp = wl_resource_get_user_data(surface);
    return static_cast<XdgSurfaceUnstableV6*>(static_cast<wayland::XdgSurfaceV6*>(tmp));
}

mf::XdgSurfaceUnstableV6::XdgSurfaceUnstableV6(wl_client* client, wl_resource* parent, uint32_t id,
                                               wl_resource* surface, std::shared_ptr<mf::Shell> const& shell,
                                               WlSeat& seat)
    : wayland::XdgSurfaceV6(client, parent, id),
      base{this, client, resource, parent, surface, shell}
{
    // must be constructed after XdgShellBase and WlAbstractMirWindow
    sink = std::make_shared<XdgSurfaceUnstableV6EventSink>(&seat, client, surface, resource, base.destroyed);
    base.sink = sink;
}

mf::XdgSurfaceUnstableV6::~XdgSurfaceUnstableV6()
{}

void mf::XdgSurfaceUnstableV6::destroy()
{
    wl_resource_destroy(resource);
}

void mf::XdgSurfaceUnstableV6::get_toplevel(uint32_t id)
{
    base.become_toplevel(id);
}

void mf::XdgSurfaceUnstableV6::get_popup(uint32_t id, struct wl_resource* parent, struct wl_resource* positioner)
{
    XdgSurfaceBase const* parent_base = &XdgSurfaceUnstableV6::from(parent)->base;
    XdgPositionerBase const& positioner_base = XdgPositionerUnstableV6::from(positioner)->base;
    base.become_popup(id, parent_base, positioner_base);
}

void mf::XdgSurfaceUnstableV6::set_window_geometry(int32_t x, int32_t y, int32_t width, int32_t height)
{
    base.set_window_geometry(x, y, width, height);
}

void mf::XdgSurfaceUnstableV6::ack_configure(uint32_t serial)
{
    base.ack_configure(serial);
}

wl_resource* mf::XdgSurfaceUnstableV6::get_resource() const
{
    return resource;
}

void mf::XdgSurfaceUnstableV6::create_toplevel(uint32_t id)
{
    new XdgToplevelUnstableV6{client, base.parent, id, base.shell, this};
}

void mf::XdgSurfaceUnstableV6::create_popup(uint32_t id)
{
    new XdgPopupUnstableV6{client, resource, id};
}

// XdgPopupUnstableV6

mf::XdgPopupUnstableV6::XdgPopupUnstableV6(struct wl_client* client, struct wl_resource* parent, uint32_t id)
    : wayland::XdgPopupV6(client, parent, id)
{}

void mf::XdgPopupUnstableV6::grab(struct wl_resource* seat, uint32_t serial)
{
    (void)seat, (void)serial;
    // TODO
}

void mf::XdgPopupUnstableV6::destroy()
{
    wl_resource_destroy(resource);
}

// XdgToplevelUnstableV6

mf::XdgToplevelUnstableV6::XdgToplevelUnstableV6(struct wl_client* client, struct wl_resource* parent, uint32_t id,
                                                 std::shared_ptr<mf::Shell> const& /*shell*/,
                                                 XdgSurfaceUnstableV6* const xdg_surface_unstable_v6)
    : wayland::XdgToplevelV6(client, parent, id),
      base{&xdg_surface_unstable_v6->base}
{
    xdg_surface_unstable_v6->sink->notify_resize = [this](geom::Size const& new_size)
        {
            wl_array states;
            wl_array_init(&states);

            zxdg_toplevel_v6_send_configure(resource, new_size.width.as_int(), new_size.height.as_int(), &states);
        };
}

void mf::XdgToplevelUnstableV6::destroy()
{
    wl_resource_destroy(resource);
}

void mf::XdgToplevelUnstableV6::set_parent(std::experimental::optional<struct wl_resource*> const& parent)
{
    if (parent && parent.value())
    {
        base->set_parent(XdgToplevelUnstableV6::from(parent.value())->base->surface_id);
    }
    else
    {
        base->set_parent({});
    }

}

void mf::XdgToplevelUnstableV6::set_title(std::string const& title)
{
    base->set_title(title);
}

void mf::XdgToplevelUnstableV6::set_app_id(std::string const& app_id)
{
    base->set_app_id(app_id);
}

void mf::XdgToplevelUnstableV6::show_window_menu(struct wl_resource* seat, uint32_t serial, int32_t x, int32_t y)
{
    base->show_window_menu(seat, serial, x, y);
}

void mf::XdgToplevelUnstableV6::move(struct wl_resource* seat, uint32_t serial)
{
    base->move(seat, serial);
}

void mf::XdgToplevelUnstableV6::resize(struct wl_resource* seat, uint32_t serial, uint32_t edges)
{
    MirResizeEdge mir_edges = mir_resize_edge_none;

    switch (edges)
    {
    case ZXDG_TOPLEVEL_V6_RESIZE_EDGE_TOP:
        mir_edges = mir_resize_edge_north;
        break;

    case ZXDG_TOPLEVEL_V6_RESIZE_EDGE_BOTTOM:
        mir_edges = mir_resize_edge_south;
        break;

    case ZXDG_TOPLEVEL_V6_RESIZE_EDGE_LEFT:
        mir_edges = mir_resize_edge_west;
        break;

    case ZXDG_TOPLEVEL_V6_RESIZE_EDGE_TOP_LEFT:
        mir_edges = mir_resize_edge_northwest;
        break;

    case ZXDG_TOPLEVEL_V6_RESIZE_EDGE_BOTTOM_LEFT:
        mir_edges = mir_resize_edge_southwest;
        break;

    case ZXDG_TOPLEVEL_V6_RESIZE_EDGE_RIGHT:
        mir_edges = mir_resize_edge_east;
        break;

    case ZXDG_TOPLEVEL_V6_RESIZE_EDGE_TOP_RIGHT:
        mir_edges = mir_resize_edge_northeast;
        break;

    case ZXDG_TOPLEVEL_V6_RESIZE_EDGE_BOTTOM_RIGHT:
        mir_edges = mir_resize_edge_southeast;
        break;

    default:;
    }

    base->resize(seat, serial, mir_edges);
}

void mf::XdgToplevelUnstableV6::set_max_size(int32_t width, int32_t height)
{
    base->set_max_size(width, height);
}

void mf::XdgToplevelUnstableV6::set_min_size(int32_t width, int32_t height)
{
    base->set_min_size(width, height);
}

void mf::XdgToplevelUnstableV6::set_maximized()
{
    base->set_maximized();
}

void mf::XdgToplevelUnstableV6::unset_maximized()
{
    base->unset_maximized();
}

void mf::XdgToplevelUnstableV6::set_fullscreen(std::experimental::optional<struct wl_resource*> const& output)
{
    base->set_fullscreen(output);
}

void mf::XdgToplevelUnstableV6::unset_fullscreen()
{
    base->unset_fullscreen();
}

void mf::XdgToplevelUnstableV6::set_minimized()
{
    base->set_minimized();
}

mf::XdgToplevelUnstableV6* mf::XdgToplevelUnstableV6::from(wl_resource* surface)
{
    auto* tmp = wl_resource_get_user_data(surface);
    return static_cast<XdgToplevelUnstableV6*>(static_cast<wayland::XdgToplevelV6*>(tmp));
}

// XdgPositionerUnstableV6

mf::XdgPositionerUnstableV6* mf::XdgPositionerUnstableV6::from(wl_resource* resource)
{
    auto* tmp = wl_resource_get_user_data(resource);
    return static_cast<XdgPositionerUnstableV6*>(static_cast<wayland::XdgPositionerV6*>(tmp));
}

mf::XdgPositionerUnstableV6::XdgPositionerUnstableV6(struct wl_client* client, struct wl_resource* parent, uint32_t id)
    : wayland::XdgPositionerV6(client, parent, id)
{}

void mf::XdgPositionerUnstableV6::destroy()
{
    wl_resource_destroy(resource);
}

void mf::XdgPositionerUnstableV6::set_size(int32_t width, int32_t height)
{
    base.size = geom::Size{width, height};
}

void mf::XdgPositionerUnstableV6::set_anchor_rect(int32_t x, int32_t y, int32_t width, int32_t height)
{
    base.aux_rect = geom::Rectangle{{x, y}, {width, height}};
}

void mf::XdgPositionerUnstableV6::set_anchor(uint32_t anchor)
{
    MirPlacementGravity placement = mir_placement_gravity_center;

    if (anchor & ZXDG_POSITIONER_V6_ANCHOR_TOP)
        placement = MirPlacementGravity(placement | mir_placement_gravity_north);

    if (anchor & ZXDG_POSITIONER_V6_ANCHOR_BOTTOM)
        placement = MirPlacementGravity(placement | mir_placement_gravity_south);

    if (anchor & ZXDG_POSITIONER_V6_ANCHOR_LEFT)
        placement = MirPlacementGravity(placement | mir_placement_gravity_west);

    if (anchor & ZXDG_POSITIONER_V6_ANCHOR_RIGHT)
        placement = MirPlacementGravity(placement | mir_placement_gravity_east);

    base.aux_rect_placement_gravity = placement;
}

void mf::XdgPositionerUnstableV6::set_gravity(uint32_t gravity)
{
    MirPlacementGravity placement = mir_placement_gravity_center;

    if (gravity & ZXDG_POSITIONER_V6_GRAVITY_TOP)
        placement = MirPlacementGravity(placement | mir_placement_gravity_south);

    if (gravity & ZXDG_POSITIONER_V6_GRAVITY_BOTTOM)
        placement = MirPlacementGravity(placement | mir_placement_gravity_north);

    if (gravity & ZXDG_POSITIONER_V6_GRAVITY_LEFT)
        placement = MirPlacementGravity(placement | mir_placement_gravity_east);

    if (gravity & ZXDG_POSITIONER_V6_GRAVITY_RIGHT)
        placement = MirPlacementGravity(placement | mir_placement_gravity_west);

    base.surface_placement_gravity = placement;
}

void mf::XdgPositionerUnstableV6::set_constraint_adjustment(uint32_t constraint_adjustment)
{
    (void)constraint_adjustment;
    // TODO
}

void mf::XdgPositionerUnstableV6::set_offset(int32_t x, int32_t y)
{
    base.aux_rect_placement_offset_x = x;
    base.aux_rect_placement_offset_y = y;
}

