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

#include "wl_surface.h"
#include "window_wl_surface_role.h"

#include "mir/shell/surface_specification.h"
#include "mir/log.h"

namespace mf = mir::frontend;
namespace geom = mir::geometry;

namespace mir
{
namespace frontend
{

class XdgSurfaceStable : wayland::XdgSurface
{
public:
    static XdgSurfaceStable* from(wl_resource* surface);

    XdgSurfaceStable(wl_client* client, wl_resource* resource_parent, uint32_t id, WlSurface* surface,
                     XdgShellStable const& xdg_shell);
    ~XdgSurfaceStable() = default;

    void destroy() override;
    void get_toplevel(uint32_t id) override;
    void get_popup(uint32_t id, std::experimental::optional<struct wl_resource*> const& parent_surface,
                   struct wl_resource* positioner) override;
    void set_window_geometry(int32_t x, int32_t y, int32_t width, int32_t height) override;
    void ack_configure(uint32_t serial) override;

    void send_configure();

    std::experimental::optional<WindowWlSurfaceRole*> const& window_role();

private:
    void set_window_role(WindowWlSurfaceRole* role);

    std::experimental::optional<WindowWlSurfaceRole*> window_role_;
    std::shared_ptr<bool> window_role_destroyed;
    WlSurface* const surface;

public:
    XdgShellStable const& xdg_shell;
};

class XdgPopupStable : wayland::XdgPopup, public WindowWlSurfaceRole
{
public:
    XdgPopupStable(struct wl_client* client, struct wl_resource* resource_parent, uint32_t id,
                   XdgSurfaceStable* xdg_surface, WlSurfaceRole* parent_role, struct wl_resource* positioner,
                   WlSurface* surface);

    void grab(struct wl_resource* seat, uint32_t serial) override;
    void destroy() override;

    void handle_resize(std::experimental::optional<geometry::Point> const& new_top_left,
                       geometry::Size const& new_size) override;

private:
    std::experimental::optional<geom::Point> cached_top_left;
    std::experimental::optional<geom::Size> cached_size;

    XdgSurfaceStable* const xdg_surface;
};

class XdgToplevelStable : wayland::XdgToplevel, public WindowWlSurfaceRole
{
public:
    XdgToplevelStable(struct wl_client* client, struct wl_resource* resource_parent, uint32_t id, XdgSurfaceStable* xdg_surface,
                      WlSurface* surface);

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

    void handle_resize(std::experimental::optional<geometry::Point> const& new_top_left,
                       geometry::Size const& new_size) override;

private:
    static XdgToplevelStable* from(wl_resource* surface);

    XdgSurfaceStable* const xdg_surface;
};

class XdgPositionerStable : public wayland::XdgPositioner, public shell::SurfaceSpecification
{
public:
    XdgPositionerStable(struct wl_client* client, struct wl_resource* parent, uint32_t id);

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

// XdgShellStable

mf::XdgShellStable::XdgShellStable(struct wl_display* display, std::shared_ptr<mf::Shell> const shell, WlSeat& seat,
                                   OutputManager* output_manager)
    : wayland::XdgWmBase(display, 1),
      shell{shell},
      seat{seat},
      output_manager{output_manager}
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
    new XdgSurfaceStable{client, resource, id, WlSurface::from(surface), *this};
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

mf::XdgSurfaceStable::XdgSurfaceStable(wl_client* client, wl_resource* resource_parent, uint32_t id, WlSurface* surface,
                                       XdgShellStable const& xdg_shell)
    : wayland::XdgSurface(client, resource_parent, id),
      surface{surface},
      xdg_shell{xdg_shell}
{
}

void mf::XdgSurfaceStable::destroy()
{
    wl_resource_destroy(resource);
}

void mf::XdgSurfaceStable::get_toplevel(uint32_t id)
{
    auto toplevel = new XdgToplevelStable{wayland::XdgSurface::client, resource, id, this, surface};
    set_window_role(toplevel);
}

void mf::XdgSurfaceStable::get_popup(uint32_t id,
                                     std::experimental::optional<struct wl_resource*> const& parent_surface,
                                     struct wl_resource* positioner)
{
    auto parent_xdg_surface = parent_surface ?
                                  std::experimental::make_optional(XdgSurfaceStable::from(parent_surface.value()))
                                  : std::experimental::nullopt;

    auto parent_window_role = parent_xdg_surface ?
                                  parent_xdg_surface.value()->window_role()
                                  : std::experimental::nullopt;


    if  (!parent_window_role)
    {
        // When we implement layer shell, it will be more clear why this function might be called with a null parent,
        // and we can revisit handling it then.
        log_warning("tried to create XDG shell stable popup without a parent");
        return;
    }

    auto popup = new XdgPopupStable{wayland::XdgSurface::client, resource, id, this, parent_window_role.value(), positioner,
                                    surface};
    set_window_role(popup);
}

void mf::XdgSurfaceStable::set_window_geometry(int32_t x, int32_t y, int32_t width, int32_t height)
{
    if (auto& role = window_role())
        role.value()->set_geometry(x, y, width, height);
}

void mf::XdgSurfaceStable::ack_configure(uint32_t serial)
{
    (void)serial;
    // TODO
}

void mf::XdgSurfaceStable::send_configure()
{
    auto const serial = wl_display_next_serial(wl_client_get_display(wayland::XdgSurface::client));
    send_configure_event(serial);
}

std::experimental::optional<mf::WindowWlSurfaceRole*> const& mf::XdgSurfaceStable::window_role()
{
    if (window_role_ && *window_role_destroyed)
    {
        window_role_ = std::experimental::nullopt;
        window_role_destroyed = nullptr;
    }

    return window_role_;
}

void mf::XdgSurfaceStable::set_window_role(WindowWlSurfaceRole* role)
{
    if (window_role())
        log_warning("XdgSurfaceStable::window_role set multiple times");

    window_role_ = role;
    window_role_destroyed = role->destroyed_flag();
}

// XdgPopupStable

mf::XdgPopupStable::XdgPopupStable(struct wl_client* client, struct wl_resource* resource_parent, uint32_t id,
                                   XdgSurfaceStable* xdg_surface,
                                   WlSurfaceRole* parent_role,
                                   struct wl_resource* positioner, WlSurface* surface)
    : wayland::XdgPopup(client, resource_parent, id),
      WindowWlSurfaceRole(&xdg_surface->xdg_shell.seat, client, surface, xdg_surface->xdg_shell.shell,
                          xdg_surface->xdg_shell.output_manager),
      xdg_surface{xdg_surface}
{
    auto specification = static_cast<mir::shell::SurfaceSpecification*>(
                                static_cast<XdgPositionerStable*>(
                                    static_cast<wayland::XdgPositioner*>(
                                        wl_resource_get_user_data(positioner))));

    specification->type = mir_window_type_freestyle;
    specification->placement_hints = mir_placement_hints_slide_any;
    specification->parent_id = parent_role->surface_id();

    apply_spec(*specification);
}

void mf::XdgPopupStable::grab(struct wl_resource* seat, uint32_t serial)
{
    (void)seat, (void)serial;
    // TODO
}

void mf::XdgPopupStable::destroy()
{
    wl_resource_destroy(resource);
}

void mf::XdgPopupStable::handle_resize(const std::experimental::optional<geometry::Point>& new_top_left,
                                       const geometry::Size& new_size)
{
    bool const needs_configure = (new_top_left != cached_top_left) || (new_size != cached_size);

    if (new_top_left)
        cached_top_left = new_top_left;

    cached_size = new_size;

    if (needs_configure && cached_top_left && cached_size)
    {
        send_configure_event(cached_top_left.value().x.as_int(),
                             cached_top_left.value().y.as_int(),
                             cached_size.value().width.as_int(),
                             cached_size.value().height.as_int());
        xdg_surface->send_configure();
    }
}

// XdgToplevelStable

mf::XdgToplevelStable::XdgToplevelStable(struct wl_client* client, struct wl_resource* resource_parent, uint32_t id,
                                         XdgSurfaceStable* xdg_surface, WlSurface* surface)
    : wayland::XdgToplevel(client, resource_parent, id),
      WindowWlSurfaceRole(&xdg_surface->xdg_shell.seat, client, surface, xdg_surface->xdg_shell.shell,
                          xdg_surface->xdg_shell.output_manager),
      xdg_surface{xdg_surface}
{
    wl_array states;
    wl_array_init(&states);
    send_configure_event(0, 0, &states);
    wl_array_release(&states);
    xdg_surface->send_configure();
}

void mf::XdgToplevelStable::destroy()
{
    wl_resource_destroy(resource);
}

void mf::XdgToplevelStable::set_parent(std::experimental::optional<struct wl_resource*> const& parent)
{
    if (parent && parent.value())
    {
        WindowWlSurfaceRole::set_parent(XdgToplevelStable::from(parent.value())->surface_id());
    }
    else
    {
        WindowWlSurfaceRole::set_parent({});
    }
}

void mf::XdgToplevelStable::set_title(std::string const& title)
{
    WindowWlSurfaceRole::set_title(title);
}

void mf::XdgToplevelStable::set_app_id(std::string const& /*app_id*/)
{
    // Logically this sets the session name, but Mir doesn't allow this (currently) and
    // allowing e.g. "session_for_client(client)->name(app_id);" would break the libmirserver ABI
}

void mf::XdgToplevelStable::show_window_menu(struct wl_resource* seat, uint32_t serial, int32_t x, int32_t y)
{
    (void)seat, (void)serial, (void)x, (void)y;
    // TODO
}

void mf::XdgToplevelStable::move(struct wl_resource* /*seat*/, uint32_t /*serial*/)
{
    initiate_interactive_move();
}

void mf::XdgToplevelStable::resize(struct wl_resource* /*seat*/, uint32_t /*serial*/, uint32_t edges)
{
    MirResizeEdge edge = mir_resize_edge_none;

    switch (edges)
    {
    case ResizeEdge::TOP:
        edge = mir_resize_edge_north;
        break;

    case ResizeEdge::BOTTOM:
        edge = mir_resize_edge_south;
        break;

    case ResizeEdge::LEFT:
        edge = mir_resize_edge_west;
        break;

    case ResizeEdge::RIGHT:
        edge = mir_resize_edge_east;
        break;

    case ResizeEdge::TOP_LEFT:
        edge = mir_resize_edge_northwest;
        break;

    case ResizeEdge::BOTTOM_LEFT:
        edge = mir_resize_edge_southwest;
        break;

    case ResizeEdge::TOP_RIGHT:
        edge = mir_resize_edge_northeast;
        break;

    case ResizeEdge::BOTTOM_RIGHT:
        edge = mir_resize_edge_southeast;
        break;

    default:;
    }

    initiate_interactive_resize(edge);
}

void mf::XdgToplevelStable::set_max_size(int32_t width, int32_t height)
{
    WindowWlSurfaceRole::set_max_size(width, height);
}

void mf::XdgToplevelStable::set_min_size(int32_t width, int32_t height)
{
    WindowWlSurfaceRole::set_min_size(width, height);
}

void mf::XdgToplevelStable::set_maximized()
{
    // We must process this request immediately (i.e. don't defer until commit())
    set_state_now(mir_window_state_maximized);
}

void mf::XdgToplevelStable::unset_maximized()
{
    // We must process this request immediately (i.e. don't defer until commit())
    set_state_now(mir_window_state_restored);
}

void mf::XdgToplevelStable::set_fullscreen(std::experimental::optional<struct wl_resource*> const& output)
{
    WindowWlSurfaceRole::set_fullscreen(output);
}

void mf::XdgToplevelStable::unset_fullscreen()
{
    // We must process this request immediately (i.e. don't defer until commit())
    // TODO: should we instead restore the previous state?
    set_state_now(mir_window_state_restored);
}

void mf::XdgToplevelStable::set_minimized()
{
    // We must process this request immediately (i.e. don't defer until commit())
    set_state_now(mir_window_state_minimized);
}

void mf::XdgToplevelStable::handle_resize(std::experimental::optional<geometry::Point> const& /*new_top_left*/,
                                          geometry::Size const& new_size)
{
    wl_array states;
    wl_array_init(&states);

    if (is_active())
    {
        if (uint32_t *state = static_cast<decltype(state)>(wl_array_add(&states, sizeof *state)))
            *state = State::ACTIVATED;
    }

    switch (window_state())
    {
    case mir_window_state_maximized:
    case mir_window_state_horizmaximized:
    case mir_window_state_vertmaximized:
        if (uint32_t *state = static_cast<decltype(state)>(wl_array_add(&states, sizeof *state)))
            *state = State::MAXIMIZED;
        break;

    case mir_window_state_fullscreen:
        if (uint32_t *state = static_cast<decltype(state)>(wl_array_add(&states, sizeof *state)))
            *state = State::FULLSCREEN;
        break;

    default:
        break;
    }

    send_configure_event(new_size.width.as_int(), new_size.height.as_int(), &states);
    wl_array_release(&states);

    xdg_surface->send_configure();
}

mf::XdgToplevelStable* mf::XdgToplevelStable::from(wl_resource* surface)
{
    auto* tmp = wl_resource_get_user_data(surface);
    return static_cast<XdgToplevelStable*>(static_cast<wayland::XdgToplevel*>(tmp));
}

// XdgPositionerStable

mf::XdgPositionerStable::XdgPositionerStable(struct wl_client* client, struct wl_resource* parent, uint32_t id)
    : wayland::XdgPositioner(client, parent, id)
{
    // specifying gravity is not required by the xdg shell protocol, but is by Mir window managers
    surface_placement_gravity = mir_placement_gravity_center;
    aux_rect_placement_gravity = mir_placement_gravity_center;
}

void mf::XdgPositionerStable::destroy()
{
    wl_resource_destroy(resource);
}

void mf::XdgPositionerStable::set_size(int32_t width, int32_t height)
{
    this->width = geom::Width{width};
    this->height = geom::Height{height};
}

void mf::XdgPositionerStable::set_anchor_rect(int32_t x, int32_t y, int32_t width, int32_t height)
{
    aux_rect = geom::Rectangle{{x, y}, {width, height}};
}

void mf::XdgPositionerStable::set_anchor(uint32_t anchor)
{
    MirPlacementGravity placement = mir_placement_gravity_center;

    switch (anchor)
    {
        case Anchor::TOP:
            placement = mir_placement_gravity_north;
            break;

        case Anchor::BOTTOM:
            placement = mir_placement_gravity_south;
            break;

        case Anchor::LEFT:
            placement = mir_placement_gravity_west;
            break;

        case Anchor::RIGHT:
            placement = mir_placement_gravity_east;
            break;

        case Anchor::TOP_LEFT:
            placement = mir_placement_gravity_northwest;
            break;

        case Anchor::BOTTOM_LEFT:
            placement = mir_placement_gravity_southwest;
            break;

        case Anchor::TOP_RIGHT:
            placement = mir_placement_gravity_northeast;
            break;

        case Anchor::BOTTOM_RIGHT:
            placement = mir_placement_gravity_southeast;
            break;

        default:
            placement = mir_placement_gravity_center;
    }

    aux_rect_placement_gravity = placement;
}

void mf::XdgPositionerStable::set_gravity(uint32_t gravity)
{
    MirPlacementGravity placement = mir_placement_gravity_center;

    switch (gravity)
    {
        case Gravity::TOP:
            placement = mir_placement_gravity_south;
            break;

        case Gravity::BOTTOM:
            placement = mir_placement_gravity_north;
            break;

        case Gravity::LEFT:
            placement = mir_placement_gravity_east;
            break;

        case Gravity::RIGHT:
            placement = mir_placement_gravity_west;
            break;

        case Gravity::TOP_LEFT:
            placement = mir_placement_gravity_southeast;
            break;

        case Gravity::BOTTOM_LEFT:
            placement = mir_placement_gravity_northeast;
            break;

        case Gravity::TOP_RIGHT:
            placement = mir_placement_gravity_southwest;
            break;

        case Gravity::BOTTOM_RIGHT:
            placement = mir_placement_gravity_northwest;
            break;

        default:
            placement = mir_placement_gravity_center;
    }

    surface_placement_gravity = placement;
}

void mf::XdgPositionerStable::set_constraint_adjustment(uint32_t constraint_adjustment)
{
    (void)constraint_adjustment;
    // TODO
}

void mf::XdgPositionerStable::set_offset(int32_t x, int32_t y)
{
    aux_rect_placement_offset_x = x;
    aux_rect_placement_offset_y = y;
}

