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

#include "xdg_shell_v6.h"

#include "wayland_utils.h"
#include "basic_surface_event_sink.h"
#include "wl_seat.h"
#include "wl_surface.h"
#include "wl_surface_role.h"

#include "mir/scene/surface_creation_parameters.h"
#include "mir/frontend/session.h"
#include "mir/scene/surface.h"
#include "mir/frontend/shell.h"
#include "mir/optional_value.h"

namespace mf = mir::frontend;
namespace geom = mir::geometry;

namespace mir
{
namespace frontend
{

class Shell;
class XdgSurfaceV6;
class WlSeat;
class XdgSurfaceV6Role;
class XdgSurfaceV6EventSink;

class XdgSurfaceV6 : wayland::XdgSurfaceV6, WlAbstractMirWindow
{
public:
    XdgSurfaceV6* get_xdgsurface(wl_resource* surface) const;

    XdgSurfaceV6(wl_client* client, wl_resource* parent, uint32_t id, wl_resource* surface,
                 std::shared_ptr<Shell> const& shell, WlSeat& seat);
    ~XdgSurfaceV6() override;

    void destroy() override;
    void get_toplevel(uint32_t id) override;
    void get_popup(uint32_t id, struct wl_resource* parent, struct wl_resource* positioner) override;
    void set_window_geometry(int32_t x, int32_t y, int32_t width, int32_t height) override;
    void ack_configure(uint32_t serial) override;
    void commit(WlSurfaceState const& state) override;

    void set_role(XdgSurfaceV6Role* role);
    void clear_role();
    void send_configure();
    void set_parent(optional_value<SurfaceId> parent_id);
    void set_title(std::string const& title);
    void move(struct wl_resource* seat, uint32_t serial);
    void resize(struct wl_resource* /*seat*/, uint32_t /*serial*/, uint32_t edges);
    void set_max_size(int32_t width, int32_t height);
    void set_min_size(int32_t width, int32_t height);
    void set_maximized();
    void unset_maximized();

    std::shared_ptr<bool> destroyed_flag();
    XdgSurfaceV6Role* role() const { return role_; }

    using WlAbstractMirWindow::client;
    using WlAbstractMirWindow::params;
    using WlAbstractMirWindow::surface_id;

private:
    XdgSurfaceV6Role* role_;

public:
    struct wl_resource* const parent;
    std::shared_ptr<Shell> const shell;
    std::shared_ptr<XdgSurfaceV6EventSink> const sink;
};

class XdgSurfaceV6EventSink : public BasicSurfaceEventSink
{
public:
    XdgSurfaceV6EventSink(WlSeat* seat, wl_client* client, wl_resource* target, wl_resource* event_sink,
                          XdgSurfaceV6* xdg_surface);
    void send_resize(geometry::Size const& new_size) const override;

private:
    XdgSurfaceV6* const xdg_surface;
};

class XdgSurfaceV6Role
{
public:
    virtual ~XdgSurfaceV6Role() = default;
    virtual void send_configure(geom::Size const& new_size, MirWindowState state, bool active) = 0;
    virtual void commit() = 0;
} extern* null_xdg_surface_v6_role_ptr;

class NullXdgSurfaceRole: public XdgSurfaceV6Role
{
    void send_configure(geom::Size const& /*new_size*/, MirWindowState /*state*/, bool /*active*/) override {}
    void commit() override {}
} null_xdg_surface_v6_role;

XdgSurfaceV6Role* null_xdg_surface_v6_role_ptr = &null_xdg_surface_v6_role;

class XdgPopupV6 : wayland::XdgPopupV6, public XdgSurfaceV6Role
{
public:
    XdgPopupV6(struct wl_client* client, struct wl_resource* parent, uint32_t id, XdgSurfaceV6* self);
    ~XdgPopupV6();

    void grab(struct wl_resource* seat, uint32_t serial) override;
    void destroy() override;

    void send_configure(geom::Size const& /*new_size*/, MirWindowState /*state*/, bool /*active*/) override {}
    void commit() override
    {
        if (first_commit)
        {
            self->send_configure();
            first_commit = false;
        }
    }

private:
    XdgSurfaceV6* const self;
    bool first_commit = true;
};

class XdgToplevelV6 : public wayland::XdgToplevelV6, public XdgSurfaceV6Role
{
public:
    XdgToplevelV6(struct wl_client* client, struct wl_resource* parent, uint32_t id,
                  std::shared_ptr<frontend::Shell> const& shell, XdgSurfaceV6* self);
    ~XdgToplevelV6();

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

    void send_configure(geom::Size const& new_size, MirWindowState state, bool active) override;
    void commit() override;

private:
    XdgToplevelV6* get_xdgtoplevel(wl_resource* surface) const;

    std::shared_ptr<frontend::Shell> const shell;
    XdgSurfaceV6* const self;
    bool first_commit = true;
};

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

}
}
namespace mf = mir::frontend;  // Keep CLion's parsing happy

// XdgShellV6

mf::XdgShellV6::XdgShellV6(struct wl_display* display, std::shared_ptr<mf::Shell> const shell, WlSeat& seat)
    : wayland::XdgShellV6(display, 1),
      shell{shell},
      seat{seat}
{}

void mf::XdgShellV6::destroy(struct wl_client* client, struct wl_resource* resource)
{
    (void)client, (void)resource;
    // TODO
}

void mf::XdgShellV6::create_positioner(struct wl_client* client, struct wl_resource* resource, uint32_t id)
{
    new XdgPositionerV6{client, resource, id};
}

void mf::XdgShellV6::get_xdg_surface(struct wl_client* client, struct wl_resource* resource, uint32_t id,
                                     struct wl_resource* surface)
{
    new XdgSurfaceV6{client, resource, id, surface, shell, seat};
}

void mf::XdgShellV6::pong(struct wl_client* client, struct wl_resource* resource, uint32_t serial)
{
    (void)client, (void)resource, (void)serial;
    // TODO
}

// XdgSurfaceV6

mf::XdgSurfaceV6* mf::XdgSurfaceV6::get_xdgsurface(wl_resource* surface) const
{
    auto* tmp = wl_resource_get_user_data(surface);
    return static_cast<XdgSurfaceV6*>(static_cast<wayland::XdgSurfaceV6*>(tmp));
}

mf::XdgSurfaceV6::XdgSurfaceV6(wl_client* client, wl_resource* parent, uint32_t id, wl_resource* surface,
                               std::shared_ptr<mf::Shell> const& shell, WlSeat& seat)
    : wayland::XdgSurfaceV6(client, parent, id),
      WlAbstractMirWindow{client, surface, resource, shell},
      role_{null_xdg_surface_v6_role_ptr},
      parent{parent},
      shell{shell},
      sink{std::make_shared<XdgSurfaceV6EventSink>(&seat, client, surface, resource, this)}
{
    WlAbstractMirWindow::sink = sink;
}

mf::XdgSurfaceV6::~XdgSurfaceV6()
{
    surface->clear_role();
}

void mf::XdgSurfaceV6::destroy()
{
    wl_resource_destroy(resource);
}

void mf::XdgSurfaceV6::get_toplevel(uint32_t id)
{
    new XdgToplevelV6{client, parent, id, shell, this};
    surface->set_role(this);
}

void mf::XdgSurfaceV6::get_popup(uint32_t id, struct wl_resource* parent, struct wl_resource* positioner)
{
    auto* tmp = wl_resource_get_user_data(positioner);
    auto const* const pos =  static_cast<XdgPositionerV6*>(static_cast<wayland::XdgPositionerV6*>(tmp));

    auto const session = get_session(client);
    auto& parent_surface = *get_xdgsurface(parent);

    params->type = mir_window_type_freestyle;
    params->parent_id = parent_surface.surface_id;
    if (pos->size.is_set()) params->size = pos->size.value();
    params->aux_rect = pos->aux_rect;
    params->surface_placement_gravity = pos->surface_placement_gravity;
    params->aux_rect_placement_gravity = pos->aux_rect_placement_gravity;
    params->aux_rect_placement_offset_x = pos->aux_rect_placement_offset_x;
    params->aux_rect_placement_offset_y = pos->aux_rect_placement_offset_y;
    params->placement_hints = mir_placement_hints_slide_any;

    new XdgPopupV6{client, parent, id, this};
    surface->set_role(this);
}

void mf::XdgSurfaceV6::set_window_geometry(int32_t x, int32_t y, int32_t width, int32_t height)
{
    geom::Displacement const buffer_offset{-x, -y};

    surface->set_buffer_offset(buffer_offset);
    window_size_ = geom::Size{width, height};

    if (surface_id.as_value())
    {
        spec().width = geom::Width{width};
        spec().height = geom::Height{height};
        invalidate_buffer_list();
    }
}

void mf::XdgSurfaceV6::ack_configure(uint32_t serial)
{
    (void)serial;
    // TODO
}

void mir::frontend::XdgSurfaceV6::commit(mir::frontend::WlSurfaceState const& state)
{
    WlAbstractMirWindow::commit(state);
    role_->commit();
}

void mf::XdgSurfaceV6::set_title(std::string const& title)
{
    if (surface_id.as_value())
    {
        spec().name = title;
    }
    else
    {
        params->name = title;
    }
}

void mf::XdgSurfaceV6::move(struct wl_resource* /*seat*/, uint32_t /*serial*/)
{
    if (surface_id.as_value())
    {
        if (auto session = get_session(client))
        {
            shell->request_operation(session, surface_id, sink->latest_timestamp(), Shell::UserRequest::move);
        }
    }
}

void mf::XdgSurfaceV6::resize(struct wl_resource* /*seat*/, uint32_t /*serial*/, uint32_t edges)
{
    if (surface_id.as_value())
    {
        if (auto session = get_session(client))
        {
            MirResizeEdge edge = mir_resize_edge_none;

            switch (edges)
            {
            case ZXDG_TOPLEVEL_V6_RESIZE_EDGE_TOP:
                edge = mir_resize_edge_north;
                break;

            case ZXDG_TOPLEVEL_V6_RESIZE_EDGE_BOTTOM:
                edge = mir_resize_edge_south;
                break;

            case ZXDG_TOPLEVEL_V6_RESIZE_EDGE_LEFT:
                edge = mir_resize_edge_west;
                break;

            case ZXDG_TOPLEVEL_V6_RESIZE_EDGE_TOP_LEFT:
                edge = mir_resize_edge_northwest;
                break;

            case ZXDG_TOPLEVEL_V6_RESIZE_EDGE_BOTTOM_LEFT:
                edge = mir_resize_edge_southwest;
                break;

            case ZXDG_TOPLEVEL_V6_RESIZE_EDGE_RIGHT:
                edge = mir_resize_edge_east;
                break;

            case ZXDG_TOPLEVEL_V6_RESIZE_EDGE_TOP_RIGHT:
                edge = mir_resize_edge_northeast;
                break;

            case ZXDG_TOPLEVEL_V6_RESIZE_EDGE_BOTTOM_RIGHT:
                edge = mir_resize_edge_southeast;
                break;

            default:;
            }

            shell->request_operation(
                session,
                surface_id,
                sink->latest_timestamp(),
                Shell::UserRequest::resize,
                edge);
        }
    }
}

void mf::XdgSurfaceV6::set_role(XdgSurfaceV6Role* role)
{
    role_ = role;
}

void mf::XdgSurfaceV6::clear_role()
{
    role_ = null_xdg_surface_v6_role_ptr;
}

void mf::XdgSurfaceV6::send_configure()
{
    auto const serial = wl_display_next_serial(wl_client_get_display(client));
    zxdg_surface_v6_send_configure(event_sink, serial);
}

void mf::XdgSurfaceV6::set_parent(optional_value<SurfaceId> parent_id)
{
    if (surface_id.as_value())
    {
        spec().parent_id = parent_id;
    }
    else
    {
        params->parent_id = parent_id;
    }
}

void mf::XdgSurfaceV6::set_max_size(int32_t width, int32_t height)
{
    if (surface_id.as_value())
    {
        if (width == 0) width = std::numeric_limits<int>::max();
        if (height == 0) height = std::numeric_limits<int>::max();

        auto& mods = spec();
        mods.max_width = geom::Width{width};
        mods.max_height = geom::Height{height};
    }
    else
    {
        if (width == 0)
        {
            if (params->max_width.is_set())
                params->max_width.consume();
        }
        else
            params->max_width = geom::Width{width};

        if (height == 0)
        {
            if (params->max_height.is_set())
                params->max_height.consume();
        }
        else
            params->max_height = geom::Height{height};
    }
}

void mf::XdgSurfaceV6::set_min_size(int32_t width, int32_t height)
{
    if (surface_id.as_value())
    {
        auto& mods = spec();
        mods.min_width = geom::Width{width};
        mods.min_height = geom::Height{height};
    }
    else
    {
        params->min_width = geom::Width{width};
        params->min_height = geom::Height{height};
    }
}

void mf::XdgSurfaceV6::set_maximized()
{
    if (surface_id.as_value())
    {
        spec().state = mir_window_state_maximized;
    }
    else
    {
        params->state = mir_window_state_maximized;
    }
}

void mf::XdgSurfaceV6::unset_maximized()
{
    if (surface_id.as_value())
    {
        spec().state = mir_window_state_restored;
    }
    else
    {
        params->state = mir_window_state_restored;
    }
}

std::shared_ptr<bool> mf::XdgSurfaceV6::destroyed_flag()
{
    return destroyed;
}

// XdgSurfaceV6EventSink

mf::XdgSurfaceV6EventSink::XdgSurfaceV6EventSink(WlSeat* seat, wl_client* client, wl_resource* target,
                                                 wl_resource* event_sink, XdgSurfaceV6* xdg_surface)
    : BasicSurfaceEventSink(seat, client, target, event_sink),
      xdg_surface{xdg_surface}
{}

void mf::XdgSurfaceV6EventSink::send_resize(geometry::Size const& new_size) const
{
    seat->spawn(run_unless(xdg_surface->destroyed_flag(), [this, new_size]()
        {
            xdg_surface->role()->send_configure(new_size, state(), is_active());
        }));
}

// XdgPopupV6

mf::XdgPopupV6::XdgPopupV6(struct wl_client* client, struct wl_resource* parent, uint32_t id, XdgSurfaceV6* self)
    : wayland::XdgPopupV6(client, parent, id),
      self{self}
{
    self->set_role(this);
}

mf::XdgPopupV6::~XdgPopupV6()
{
    self->clear_role();
}

void mf::XdgPopupV6::grab(struct wl_resource* seat, uint32_t serial)
{
    (void)seat, (void)serial;
    // TODO
}

void mf::XdgPopupV6::destroy()
{
    wl_resource_destroy(resource);
}

// XdgToplevelV6

mf::XdgToplevelV6::XdgToplevelV6(struct wl_client* client, struct wl_resource* parent, uint32_t id,
                                 std::shared_ptr<mf::Shell> const& shell, XdgSurfaceV6* self)
    : wayland::XdgToplevelV6(client, parent, id),
      shell{shell},
      self{self}
{
    self->set_role(this);
}

mf::XdgToplevelV6::~XdgToplevelV6()
{
    self->clear_role();
}

void mf::XdgToplevelV6::destroy()
{
    wl_resource_destroy(resource);
}

void mf::XdgToplevelV6::set_parent(std::experimental::optional<struct wl_resource*> const& parent)
{
    if (parent && parent.value())
    {
        self->set_parent(get_xdgtoplevel(parent.value())->self->surface_id);
    }
    else
    {
        self->set_parent({});
    }

}

void mf::XdgToplevelV6::set_title(std::string const& title)
{
    self->set_title(title);
}

void mf::XdgToplevelV6::set_app_id(std::string const& /*app_id*/)
{
    // Logically this sets the session name, but Mir doesn't allow this (currently) and
    // allowing e.g. "session_for_client(client)->name(app_id);" would break the libmirserver ABI
}

void mf::XdgToplevelV6::show_window_menu(struct wl_resource* seat, uint32_t serial, int32_t x, int32_t y)
{
    (void)seat, (void)serial, (void)x, (void)y;
    // TODO
}

void mf::XdgToplevelV6::move(struct wl_resource* seat, uint32_t serial)
{
    self->move(seat, serial);
}

void mf::XdgToplevelV6::resize(struct wl_resource* seat, uint32_t serial, uint32_t edges)
{
    self->resize(seat, serial, edges);
}

void mf::XdgToplevelV6::set_max_size(int32_t width, int32_t height)
{
    self->set_max_size(width, height);
}

void mf::XdgToplevelV6::set_min_size(int32_t width, int32_t height)
{
    self->set_min_size(width, height);
}

void mf::XdgToplevelV6::set_maximized()
{
    self->set_maximized();
}

void mf::XdgToplevelV6::unset_maximized()
{
    self->unset_maximized();
}

void mf::XdgToplevelV6::set_fullscreen(std::experimental::optional<struct wl_resource*> const& output)
{
    (void)output;
    // TODO
}

void mf::XdgToplevelV6::unset_fullscreen()
{
    // TODO
}

void mf::XdgToplevelV6::set_minimized()
{
    // TODO
}

void mf::XdgToplevelV6::send_configure(geom::Size const& new_size, MirWindowState state, bool active)
{
    wl_array states;
    wl_array_init(&states);

    if (active)
    {
        if (uint32_t *state = static_cast<decltype(state)>(wl_array_add(&states, sizeof *state)))
            *state = ZXDG_TOPLEVEL_V6_STATE_ACTIVATED;
    }

    switch (state)
    {
    case mir_window_state_maximized:
    case mir_window_state_horizmaximized:
    case mir_window_state_vertmaximized:
        if (uint32_t *state = static_cast<decltype(state)>(wl_array_add(&states, sizeof *state)))
            *state = ZXDG_TOPLEVEL_V6_STATE_MAXIMIZED;
        break;

    case mir_window_state_fullscreen:
        if (uint32_t *state = static_cast<decltype(state)>(wl_array_add(&states, sizeof *state)))
            *state = ZXDG_TOPLEVEL_V6_STATE_FULLSCREEN;
        break;

    default:
        break;
    }

    zxdg_toplevel_v6_send_configure(resource, new_size.width.as_int(), new_size.height.as_int(), &states);

    wl_array_release(&states);

    self->send_configure();
}

void mf::XdgToplevelV6::commit()
{
    if (first_commit)
    {
        wl_array states;
        wl_array_init(&states);
        zxdg_toplevel_v6_send_configure(resource, 0, 0, &states);
        wl_array_release(&states);
        self->send_configure();
        first_commit = false;
    }
}

mf::XdgToplevelV6* mf::XdgToplevelV6::get_xdgtoplevel(wl_resource* surface) const
{
    auto* tmp = wl_resource_get_user_data(surface);
    return static_cast<XdgToplevelV6*>(static_cast<wayland::XdgToplevelV6*>(tmp));
}

// XdgPositionerV6

mf::XdgPositionerV6::XdgPositionerV6(struct wl_client* client, struct wl_resource* parent, uint32_t id)
    : wayland::XdgPositionerV6(client, parent, id)
{}

void mf::XdgPositionerV6::destroy()
{
    wl_resource_destroy(resource);
}

void mf::XdgPositionerV6::set_size(int32_t width, int32_t height)
{
    size = geom::Size{width, height};
}

void mf::XdgPositionerV6::set_anchor_rect(int32_t x, int32_t y, int32_t width, int32_t height)
{
    aux_rect = geom::Rectangle{{x, y}, {width, height}};
}

void mf::XdgPositionerV6::set_anchor(uint32_t anchor)
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

    aux_rect_placement_gravity = placement;
}

void mf::XdgPositionerV6::set_gravity(uint32_t gravity)
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

    surface_placement_gravity = placement;
}

void mf::XdgPositionerV6::set_constraint_adjustment(uint32_t constraint_adjustment)
{
    (void)constraint_adjustment;
    // TODO
}

void mf::XdgPositionerV6::set_offset(int32_t x, int32_t y)
{
    aux_rect_placement_offset_x = x;
    aux_rect_placement_offset_y = y;
}

