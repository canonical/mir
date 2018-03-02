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

#include "xdg_shell_stable.h"
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
class XdgSurfaceStable;
class WlSeat;
class XdgSurfaceStableEventSink;

class XdgSurfaceStable : XdgSurfaceBase::AdapterInterface, wayland::XdgSurface
{
public:
    static XdgSurfaceStable* from(wl_resource* surface);

    XdgSurfaceStable(wl_client* client, wl_resource* parent, uint32_t id, wl_resource* surface,
                         std::shared_ptr<Shell> const& shell, WlSeat& seat);
    ~XdgSurfaceStable() override;

    // from wayland::XdgSurface
    void destroy() override;
    void get_toplevel(uint32_t id) override;
    void get_popup(uint32_t id, std::experimental::optional<struct wl_resource*> const& parent,
                   struct wl_resource* positioner) override;
    void set_window_geometry(int32_t x, int32_t y, int32_t width, int32_t height) override;
    void ack_configure(uint32_t serial) override;

    // from XdgSurfaceBase::AdapterInterface
    wl_resource* get_resource() const override;
    void create_toplevel(uint32_t id) override;
    void create_popup(uint32_t id) override;

    std::shared_ptr<XdgSurfaceStableEventSink> sink;
    XdgSurfaceBase base;
};

class XdgPopupStable : wayland::XdgPopup
{
public:
    XdgPopupStable(struct wl_client* client, struct wl_resource* parent, uint32_t id);

    void grab(struct wl_resource* seat, uint32_t serial) override;
    void destroy() override;
};

class XdgToplevelStable : public wayland::XdgToplevel
{
public:
    XdgToplevelStable(struct wl_client* client, struct wl_resource* parent, uint32_t id,
                          std::shared_ptr<frontend::Shell> const& shell, XdgSurfaceStable* const xdg_surface);

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
    static XdgToplevelStable* from(wl_resource* surface);

    XdgSurfaceBase* const base;
};

class XdgPositionerStable : public wayland::XdgPositioner
{
public:
    static XdgPositionerStable* from(wl_resource* resource);

    XdgPositionerStable(struct wl_client* client, struct wl_resource* parent, uint32_t id);

    void destroy() override;
    void set_size(int32_t width, int32_t height) override;
    void set_anchor_rect(int32_t x, int32_t y, int32_t width, int32_t height) override;
    void set_anchor(uint32_t anchor) override;
    void set_gravity(uint32_t gravity) override;
    void set_constraint_adjustment(uint32_t constraint_adjustment) override;
    void set_offset(int32_t x, int32_t y) override;

    XdgPositionerBase base;
};

class XdgSurfaceStableEventSink : public BasicSurfaceEventSink
{
public:
    using BasicSurfaceEventSink::BasicSurfaceEventSink;

    XdgSurfaceStableEventSink(WlSeat* seat, wl_client* client, wl_resource* target, wl_resource* xdg_surface_stable,
                                  std::shared_ptr<bool> const& destroyed)
        : BasicSurfaceEventSink(seat, client, target, destroyed),
          xdg_surface_stable{xdg_surface_stable}
    {
        run_unless_destroyed([this]()
            {
                uint32_t serial = wl_display_next_serial(wl_client_get_display(this->client));
                xdg_surface_send_configure(this->xdg_surface_stable, serial);
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
                    xdg_surface_send_configure(xdg_surface_stable, serial);
                });
        }
    }

    std::function<void(geometry::Size const& new_size)> notify_resize = [](auto){};

private:
    wl_resource* xdg_surface_stable;
};

}
}

// XdgShellStable

mf::XdgShellStable::XdgShellStable(struct wl_display* display, std::shared_ptr<mf::Shell> const shell, WlSeat& seat)
    : wayland::XdgWmBase(display, 1),
      shell{shell},
      seat{seat}
{}

void mf::XdgShellStable::destroy(struct wl_client* client, struct wl_resource* resource)
{
    (void)client, (void)resource;
    // TODO
}

void mf::XdgShellStable::create_positioner(struct wl_client* client, struct wl_resource* resource, uint32_t id)
{
    new XdgPositionerStable{client, resource, id};
}

void mf::XdgShellStable::get_xdg_surface(struct wl_client* client, struct wl_resource* resource, uint32_t id,
                                     struct wl_resource* surface)
{
    new XdgSurfaceStable{client, resource, id, surface, shell, seat};
}

void mf::XdgShellStable::pong(struct wl_client* client, struct wl_resource* resource, uint32_t serial)
{
    (void)client, (void)resource, (void)serial;
    // TODO
}

// XdgSurfaceStable

mf::XdgSurfaceStable* mf::XdgSurfaceStable::from(wl_resource* surface)
{
    auto* tmp = wl_resource_get_user_data(surface);
    return static_cast<XdgSurfaceStable*>(static_cast<wayland::XdgSurface*>(tmp));
}

mf::XdgSurfaceStable::XdgSurfaceStable(wl_client* client, wl_resource* parent, uint32_t id, wl_resource* surface,
                                       std::shared_ptr<mf::Shell> const& shell, WlSeat& seat)
    : wayland::XdgSurface(client, parent, id),
      base{this, client, resource, parent, surface, shell}
{
    // must be constructed after XdgShellBase and WlAbstractMirWindow
    sink = std::make_shared<XdgSurfaceStableEventSink>(&seat, client, surface, resource, base.destroyed);
    base.sink = sink;
}

mf::XdgSurfaceStable::~XdgSurfaceStable()
{}

void mf::XdgSurfaceStable::destroy()
{
    wl_resource_destroy(resource);
}

void mf::XdgSurfaceStable::get_toplevel(uint32_t id)
{
    base.become_toplevel(id);
}

void mf::XdgSurfaceStable::get_popup(uint32_t id, std::experimental::optional<wl_resource*> const& parent,
                                     wl_resource* positioner)
{
    std::experimental::optional<XdgSurfaceBase const*> parent_base = std::experimental::nullopt;
    if (parent)
        parent_base = &XdgSurfaceStable::from(parent.value())->base;
    XdgPositionerBase const& positioner_base = XdgPositionerStable::from(positioner)->base;
    base.become_popup(id, parent_base, positioner_base);
}

void mf::XdgSurfaceStable::set_window_geometry(int32_t x, int32_t y, int32_t width, int32_t height)
{
    base.set_window_geometry(x, y, width, height);
}

void mf::XdgSurfaceStable::ack_configure(uint32_t serial)
{
    base.ack_configure(serial);
}

wl_resource* mf::XdgSurfaceStable::get_resource() const
{
    return resource;
}

void mf::XdgSurfaceStable::create_toplevel(uint32_t id)
{
    new XdgToplevelStable{client, base.parent, id, base.shell, this};
}

void mf::XdgSurfaceStable::create_popup(uint32_t id)
{
    new XdgPopupStable{client, resource, id};
}

// XdgPopupStable

mf::XdgPopupStable::XdgPopupStable(struct wl_client* client, struct wl_resource* parent, uint32_t id)
    : wayland::XdgPopup(client, parent, id)
{}

void mf::XdgPopupStable::grab(struct wl_resource* seat, uint32_t serial)
{
    (void)seat, (void)serial;
    // TODO
}

void mf::XdgPopupStable::destroy()
{
    wl_resource_destroy(resource);
}

// XdgToplevelStable

mf::XdgToplevelStable::XdgToplevelStable(struct wl_client* client, struct wl_resource* parent, uint32_t id,
                                         std::shared_ptr<mf::Shell> const& /*shell*/,
                                         XdgSurfaceStable* const xdg_surface_stable)
    : wayland::XdgToplevel(client, parent, id),
      base{&xdg_surface_stable->base}
{
    xdg_surface_stable->sink->notify_resize = [this](geom::Size const& new_size)
        {
            wl_array states;
            wl_array_init(&states);

            xdg_toplevel_send_configure(resource, new_size.width.as_int(), new_size.height.as_int(), &states);
        };
}

void mf::XdgToplevelStable::destroy()
{
    wl_resource_destroy(resource);
}

void mf::XdgToplevelStable::set_parent(std::experimental::optional<struct wl_resource*> const& parent)
{
    if (parent && parent.value())
    {
        base->set_parent(XdgToplevelStable::from(parent.value())->base->surface_id);
    }
    else
    {
        base->set_parent({});
    }

}

void mf::XdgToplevelStable::set_title(std::string const& title)
{
    base->set_title(title);
}

void mf::XdgToplevelStable::set_app_id(std::string const& app_id)
{
    base->set_app_id(app_id);
}

void mf::XdgToplevelStable::show_window_menu(struct wl_resource* seat, uint32_t serial, int32_t x, int32_t y)
{
    base->show_window_menu(seat, serial, x, y);
}

void mf::XdgToplevelStable::move(struct wl_resource* seat, uint32_t serial)
{
    base->move(seat, serial);
}

void mf::XdgToplevelStable::resize(struct wl_resource* seat, uint32_t serial, uint32_t edges)
{
    MirResizeEdge mir_edges = mir_resize_edge_none;

    switch (edges)
    {
    case XDG_TOPLEVEL_RESIZE_EDGE_TOP:
        mir_edges = mir_resize_edge_north;
        break;

    case XDG_TOPLEVEL_RESIZE_EDGE_BOTTOM:
        mir_edges = mir_resize_edge_south;
        break;

    case XDG_TOPLEVEL_RESIZE_EDGE_LEFT:
        mir_edges = mir_resize_edge_west;
        break;

    case XDG_TOPLEVEL_RESIZE_EDGE_TOP_LEFT:
        mir_edges = mir_resize_edge_northwest;
        break;

    case XDG_TOPLEVEL_RESIZE_EDGE_BOTTOM_LEFT:
        mir_edges = mir_resize_edge_southwest;
        break;

    case XDG_TOPLEVEL_RESIZE_EDGE_RIGHT:
        mir_edges = mir_resize_edge_east;
        break;

    case XDG_TOPLEVEL_RESIZE_EDGE_TOP_RIGHT:
        mir_edges = mir_resize_edge_northeast;
        break;

    case XDG_TOPLEVEL_RESIZE_EDGE_BOTTOM_RIGHT:
        mir_edges = mir_resize_edge_southeast;
        break;
    }

    base->resize(seat, serial, mir_edges);
}

void mf::XdgToplevelStable::set_max_size(int32_t width, int32_t height)
{
    base->set_max_size(width, height);
}

void mf::XdgToplevelStable::set_min_size(int32_t width, int32_t height)
{
    base->set_min_size(width, height);
}

void mf::XdgToplevelStable::set_maximized()
{
    base->set_maximized();
}

void mf::XdgToplevelStable::unset_maximized()
{
    base->unset_maximized();
}

void mf::XdgToplevelStable::set_fullscreen(std::experimental::optional<struct wl_resource*> const& output)
{
    base->set_fullscreen(output);
}

void mf::XdgToplevelStable::unset_fullscreen()
{
    base->unset_fullscreen();
}

void mf::XdgToplevelStable::set_minimized()
{
    base->set_minimized();
}

mf::XdgToplevelStable* mf::XdgToplevelStable::from(wl_resource* surface)
{
    auto* tmp = wl_resource_get_user_data(surface);
    return static_cast<XdgToplevelStable*>(static_cast<wayland::XdgToplevel*>(tmp));
}

// XdgPositionerStable

mf::XdgPositionerStable* mf::XdgPositionerStable::from(wl_resource* resource)
{
    auto* tmp = wl_resource_get_user_data(resource);
    return static_cast<XdgPositionerStable*>(static_cast<wayland::XdgPositioner*>(tmp));
}

mf::XdgPositionerStable::XdgPositionerStable(struct wl_client* client, struct wl_resource* parent, uint32_t id)
    : wayland::XdgPositioner(client, parent, id)
{}

void mf::XdgPositionerStable::destroy()
{
    wl_resource_destroy(resource);
}

void mf::XdgPositionerStable::set_size(int32_t width, int32_t height)
{
    base.size = geom::Size{width, height};
}

void mf::XdgPositionerStable::set_anchor_rect(int32_t x, int32_t y, int32_t width, int32_t height)
{
    base.aux_rect = geom::Rectangle{{x, y}, {width, height}};
}

void mf::XdgPositionerStable::set_anchor(uint32_t anchor)
{
    MirPlacementGravity placement = mir_placement_gravity_center;

    switch (anchor)
    {
    case XDG_POSITIONER_ANCHOR_TOP:
        placement = mir_placement_gravity_north;
        break;

    case XDG_POSITIONER_ANCHOR_BOTTOM:
        placement = mir_placement_gravity_south;
        break;

    case XDG_POSITIONER_ANCHOR_LEFT:
        placement = mir_placement_gravity_west;
        break;

    case XDG_POSITIONER_ANCHOR_RIGHT:
        placement = mir_placement_gravity_east;
        break;

    case XDG_POSITIONER_ANCHOR_TOP_LEFT:
        placement = mir_placement_gravity_northwest;
        break;

    case XDG_POSITIONER_ANCHOR_TOP_RIGHT:
        placement = mir_placement_gravity_northeast;
        break;

    case XDG_POSITIONER_ANCHOR_BOTTOM_LEFT:
        placement = mir_placement_gravity_southwest;
        break;

    case XDG_POSITIONER_ANCHOR_BOTTOM_RIGHT:
        placement = mir_placement_gravity_southeast;
        break;
    }

    base.surface_placement_gravity = placement;
}

void mf::XdgPositionerStable::set_gravity(uint32_t gravity)
{
    MirPlacementGravity placement = mir_placement_gravity_center;

    switch (gravity)
    {
    case XDG_POSITIONER_GRAVITY_TOP:
        placement = mir_placement_gravity_north;
        break;

    case XDG_POSITIONER_GRAVITY_BOTTOM:
        placement = mir_placement_gravity_south;
        break;

    case XDG_POSITIONER_GRAVITY_LEFT:
        placement = mir_placement_gravity_west;
        break;

    case XDG_POSITIONER_GRAVITY_RIGHT:
        placement = mir_placement_gravity_east;
        break;

    case XDG_POSITIONER_GRAVITY_TOP_LEFT:
        placement = mir_placement_gravity_northwest;
        break;

    case XDG_POSITIONER_GRAVITY_TOP_RIGHT:
        placement = mir_placement_gravity_northeast;
        break;

    case XDG_POSITIONER_GRAVITY_BOTTOM_LEFT:
        placement = mir_placement_gravity_southwest;
        break;

    case XDG_POSITIONER_GRAVITY_BOTTOM_RIGHT:
        placement = mir_placement_gravity_southeast;
        break;
    }

    base.aux_rect_placement_gravity = placement;
}

void mf::XdgPositionerStable::set_constraint_adjustment(uint32_t constraint_adjustment)
{
    (void)constraint_adjustment;
    // TODO
}

void mf::XdgPositionerStable::set_offset(int32_t x, int32_t y)
{
    base.aux_rect_placement_offset_x = x;
    base.aux_rect_placement_offset_y = y;
}

