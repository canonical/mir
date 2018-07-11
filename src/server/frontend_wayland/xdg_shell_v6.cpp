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
#include "wl_surface_event_sink.h"
#include "wl_seat.h"
#include "wl_surface.h"
#include "window_wl_surface_role.h"

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

class XdgSurfaceV6 : wayland::XdgSurfaceV6, public WindowWlSurfaceRole
{
public:
    static XdgSurfaceV6* from(wl_resource* surface);

    XdgSurfaceV6(wl_client* client, wl_resource* parent, uint32_t id, WlSurface* surface,
                 std::shared_ptr<Shell> const& shell, WlSeat& seat, OutputManager* output_manager);
    ~XdgSurfaceV6() = default;

    void destroy() override;
    void get_toplevel(uint32_t id) override;
    void get_popup(uint32_t id, struct wl_resource* parent, struct wl_resource* positioner) override;
    void set_window_geometry(int32_t x, int32_t y, int32_t width, int32_t height) override;
    void ack_configure(uint32_t serial) override;
    void commit(WlSurfaceState const& state) override;

    void handle_resize(const geometry::Size & new_size) override;

    void set_parent(optional_value<SurfaceId> parent_id);
    void set_title(std::string const& title);
    void move(struct wl_resource* seat, uint32_t serial);
    void resize(struct wl_resource* /*seat*/, uint32_t /*serial*/, uint32_t edges);
    void set_notify_resize(std::function<void(geometry::Size const& new_size, MirWindowState state, bool active)> notify_resize);
    void set_next_commit_action(std::function<void()> action);
    void clear_next_commit_action();
    void set_max_size(int32_t width, int32_t height);
    void set_min_size(int32_t width, int32_t height);

    using WindowWlSurfaceRole::client;

    struct wl_resource* const parent;
    std::shared_ptr<Shell> const shell;
    std::function<void()> next_commit_action{[]{}};
    std::function<void(geometry::Size const& new_size, MirWindowState state, bool active)> notify_resize =
        [](auto, auto, auto){};
};

class XdgPopupV6 : wayland::XdgPopupV6
{
public:
    XdgPopupV6(struct wl_client* client, struct wl_resource* parent, uint32_t id, XdgSurfaceV6* self);

    void grab(struct wl_resource* seat, uint32_t serial) override;
    void destroy() override;
};

class XdgToplevelV6 : public wayland::XdgToplevelV6
{
public:
    XdgToplevelV6(struct wl_client* client, struct wl_resource* parent, uint32_t id,
                  std::shared_ptr<frontend::Shell> const& shell, XdgSurfaceV6* self);

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
    static XdgToplevelV6* from(wl_resource* surface);

    std::shared_ptr<frontend::Shell> const shell;
    XdgSurfaceV6* const self;
};

class XdgPositionerV6 : public wayland::XdgPositionerV6, public WindowPositionerData
{
public:
    XdgPositionerV6(struct wl_client* client, struct wl_resource* parent, uint32_t id);

private:
    void destroy() override;
    void set_size(int32_t width, int32_t height) override;
    void set_anchor_rect(int32_t x, int32_t y, int32_t width, int32_t height) override;
    void set_anchor(uint32_t anchor) override;
    void set_gravity(uint32_t gravity) override;
    void set_constraint_adjustment(uint32_t constraint_adjustment) override;
    void set_offset(int32_t x, int32_t y) override;
};

}
}
namespace mf = mir::frontend;  // Keep CLion's parsing happy

// XdgShellV6

mf::XdgShellV6::XdgShellV6(
    struct wl_display* display,
    std::shared_ptr<mf::Shell> const shell,
    WlSeat& seat,
    OutputManager* output_manager) :
    wayland::XdgShellV6(display, 1),
    shell{shell},
    seat{seat},
    output_manager{output_manager}
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
    new XdgSurfaceV6{client, resource, id, WlSurface::from(surface), shell, seat, output_manager};
}

void mf::XdgShellV6::pong(struct wl_client* client, struct wl_resource* resource, uint32_t serial)
{
    (void)client, (void)resource, (void)serial;
    // TODO
}

// XdgSurfaceV6

mf::XdgSurfaceV6* mf::XdgSurfaceV6::from(wl_resource* surface)
{
    auto* tmp = wl_resource_get_user_data(surface);
    return static_cast<XdgSurfaceV6*>(static_cast<wayland::XdgSurfaceV6*>(tmp));
}

mf::XdgSurfaceV6::XdgSurfaceV6(wl_client* client, wl_resource* parent, uint32_t id, WlSurface* surface,
                               std::shared_ptr<mf::Shell> const& shell, WlSeat& seat, OutputManager* output_manager)
    : wayland::XdgSurfaceV6(client, parent, id),
      WindowWlSurfaceRole{&seat, client, surface, shell, output_manager},
      parent{parent},
      shell{shell}
{
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
    auto const* const pos = static_cast<WindowPositionerData*>(
                                static_cast<XdgPositionerV6*>(
                                    static_cast<wayland::XdgPositionerV6*>(
                                        wl_resource_get_user_data(positioner))));

    auto& parent_surface = *XdgSurfaceV6::from(parent);

    params->type = mir_window_type_freestyle;
    params->parent_id = parent_surface.surface_id();

    pos->apply_to(*params);

    new XdgPopupV6{client, parent, id, this};
    surface->set_role(this);
}

void mf::XdgSurfaceV6::set_window_geometry(int32_t x, int32_t y, int32_t width, int32_t height)
{
    WindowWlSurfaceRole::set_geometry(x, y, width, height);
}

void mf::XdgSurfaceV6::ack_configure(uint32_t serial)
{
    (void)serial;
    // TODO
}


void mf::XdgSurfaceV6::set_title(std::string const& title)
{
    WindowWlSurfaceRole::set_title(title);
}

void mf::XdgSurfaceV6::move(struct wl_resource* /*seat*/, uint32_t /*serial*/)
{
    WindowWlSurfaceRole::initiate_interactive_move();
}

void mf::XdgSurfaceV6::resize(struct wl_resource* /*seat*/, uint32_t /*serial*/, uint32_t edges)
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

    WindowWlSurfaceRole::initiate_interactive_resize(edge);
}

void mf::XdgSurfaceV6::set_notify_resize(std::function<void(geometry::Size const&, MirWindowState, bool)> notify_resize_)
{
    notify_resize = notify_resize_;
}

void mf::XdgSurfaceV6::set_parent(optional_value<SurfaceId> parent_id)
{
    WindowWlSurfaceRole::set_parent(parent_id);
}

void mf::XdgSurfaceV6::set_max_size(int32_t width, int32_t height)
{
    WindowWlSurfaceRole::set_max_size(width, height);
}

void mf::XdgSurfaceV6::set_min_size(int32_t width, int32_t height)
{
    WindowWlSurfaceRole::set_min_size(width, height);
}

void mf::XdgSurfaceV6::clear_next_commit_action()
{
    next_commit_action = []{};
}

void mf::XdgSurfaceV6::set_next_commit_action(std::function<void()> action)
{
    next_commit_action = [this, action]
    {
        action();
        auto const serial = wl_display_next_serial(wl_client_get_display(client));
        zxdg_surface_v6_send_configure(resource, serial);
    };
}

void mf::XdgSurfaceV6::commit(mf::WlSurfaceState const& state)
{
    WindowWlSurfaceRole::commit(state);
    next_commit_action();
    clear_next_commit_action();
}

void mf::XdgSurfaceV6::handle_resize(geometry::Size const& new_size)
{
    auto const action = [notify_resize=notify_resize, new_size, sink=sink]
        { notify_resize(new_size, sink->state(), sink->is_active()); };

    auto const serial = wl_display_next_serial(wl_client_get_display(client));
    action();
    zxdg_surface_v6_send_configure(resource, serial);

    set_next_commit_action(action);
}

// XdgPopupV6

mf::XdgPopupV6::XdgPopupV6(struct wl_client* client, struct wl_resource* parent, uint32_t id, XdgSurfaceV6* self)
    : wayland::XdgPopupV6(client, parent, id)
{
    // TODO Make this readable!!! This "works" by exploiting the non-obvious side-effect
    // of causing a zxdg_surface_v6_send_configure() event to become pending.
    self->set_next_commit_action([]{});
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
    self->set_notify_resize(
        [this](geom::Size const& new_size, MirWindowState state, bool active)
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
        });

    self->set_next_commit_action(
        [resource = this->resource]
            {
                wl_array states;
                wl_array_init(&states);
                zxdg_toplevel_v6_send_configure(resource, 0, 0, &states);
                wl_array_release(&states);
            });
}

void mf::XdgToplevelV6::destroy()
{
    wl_resource_destroy(resource);
}

void mf::XdgToplevelV6::set_parent(std::experimental::optional<struct wl_resource*> const& parent)
{
    if (parent && parent.value())
    {
        self->set_parent(XdgToplevelV6::from(parent.value())->self->surface_id());
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
    self->set_fullscreen(output);
}

void mf::XdgToplevelV6::unset_fullscreen()
{
    self->unset_fullscreen();
}

void mf::XdgToplevelV6::set_minimized()
{
    self->set_minimized();
}

mf::XdgToplevelV6* mf::XdgToplevelV6::from(wl_resource* surface)
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

