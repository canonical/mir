/*
 * Copyright © 2018-2019 Canonical Ltd.
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

#include "mir/frontend/session.h"
#include "mir/frontend/wayland.h"
#include "mir/shell/surface_specification.h"
#include "mir/log.h"

#include <boost/throw_exception.hpp>

namespace mf = mir::frontend;
namespace geom = mir::geometry;
namespace mw = mir::wayland;

namespace mir
{
namespace frontend
{

class XdgSurfaceStable : wayland::XdgSurface
{
public:
    static XdgSurfaceStable* from(wl_resource* surface);

    XdgSurfaceStable(wl_resource* new_resource, WlSurface* surface, XdgShellStable const& xdg_shell);
    ~XdgSurfaceStable() = default;

    void destroy() override;
    void get_toplevel(wl_resource* new_toplevel) override;
    void get_popup(
        wl_resource* new_popup,
        std::experimental::optional<wl_resource*> const& parent_surface,
        wl_resource* positioner) override;
    void set_window_geometry(int32_t x, int32_t y, int32_t width, int32_t height) override;
    void ack_configure(uint32_t serial) override;

    void send_configure();

    std::experimental::optional<WindowWlSurfaceRole*> const& window_role();

    using wayland::XdgSurface::client;
    using wayland::XdgSurface::resource;

private:
    void set_window_role(WindowWlSurfaceRole* role);

    std::experimental::optional<WindowWlSurfaceRole*> window_role_;
    std::shared_ptr<bool> window_role_destroyed;
    WlSurface* const surface;

public:
    XdgShellStable const& xdg_shell;
};

class XdgToplevelStable : wayland::XdgToplevel, public WindowWlSurfaceRole
{
public:
    XdgToplevelStable(
        wl_resource* new_resource,
        XdgSurfaceStable* xdg_surface,
        WlSurface* surface);

    void destroy() override;
    void set_parent(std::experimental::optional<struct wl_resource*> const& parent) override;
    void set_title(std::string const& title) override;
    void set_app_id(std::string const& app_id) override;
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

    void handle_commit() override {};
    void handle_state_change(MirWindowState /*new_state*/) override;
    void handle_active_change(bool /*is_now_active*/) override;
    void handle_resize(std::experimental::optional<geometry::Point> const& new_top_left,
                       geometry::Size const& new_size) override;
    void handle_close_request() override;

private:
    static XdgToplevelStable* from(wl_resource* surface);
    void send_toplevel_configure();

    XdgSurfaceStable* const xdg_surface;
};

class XdgPositionerStable : public wayland::XdgPositioner, public shell::SurfaceSpecification
{
public:
    XdgPositionerStable(wl_resource* new_resource);

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

class mf::XdgShellStable::Instance : public wayland::XdgWmBase
{
public:
    Instance(wl_resource* new_resource, mf::XdgShellStable* shell);

private:
    void destroy() override;
    void create_positioner(wl_resource* new_positioner) override;
    void get_xdg_surface(wl_resource* new_xdg_surface, wl_resource* surface) override;
    void pong(uint32_t serial) override;

    mf::XdgShellStable* const shell;
};

// XdgShellStable

mf::XdgShellStable::XdgShellStable(struct wl_display* display, std::shared_ptr<mf::Shell> const shell, WlSeat& seat,
                                   OutputManager* output_manager)
    : Global(display, Version<1>()),
      shell{shell},
      seat{seat},
      output_manager{output_manager}
{
}

void mf::XdgShellStable::bind(wl_resource* new_resource)
{
    new Instance{new_resource, this};
}

mf::XdgShellStable::Instance::Instance(wl_resource* new_resource, mf::XdgShellStable* shell)
    : XdgWmBase{new_resource, Version<1>()},
      shell{shell}
{
}

void mf::XdgShellStable::Instance::destroy()
{
    destroy_wayland_object();
}

void mf::XdgShellStable::Instance::create_positioner(wl_resource* new_positioner)
{
    new XdgPositionerStable{new_positioner};
}

void mf::XdgShellStable::Instance::get_xdg_surface(wl_resource* new_shell_surface, wl_resource* surface)
{
    new XdgSurfaceStable{new_shell_surface, WlSurface::from(surface), *shell};
}

void mf::XdgShellStable::Instance::pong(uint32_t serial)
{
    (void)serial;
    // TODO
}

// XdgSurfaceStable

mf::XdgSurfaceStable* mf::XdgSurfaceStable::from(wl_resource* surface)
{
    auto* tmp = wl_resource_get_user_data(surface);
    return static_cast<XdgSurfaceStable*>(static_cast<wayland::XdgSurface*>(tmp));
}

mf::XdgSurfaceStable::XdgSurfaceStable(wl_resource* new_resource, WlSurface* surface, XdgShellStable const& xdg_shell)
    : mw::XdgSurface(new_resource, Version<1>()),
      surface{surface},
      xdg_shell{xdg_shell}
{
}

void mf::XdgSurfaceStable::destroy()
{
    destroy_wayland_object();
}

void mf::XdgSurfaceStable::get_toplevel(wl_resource* new_toplevel)
{
    auto toplevel = new XdgToplevelStable{new_toplevel, this, surface};
    set_window_role(toplevel);
}

void mf::XdgSurfaceStable::get_popup(
    wl_resource* new_popup,
    std::experimental::optional<struct wl_resource*> const& parent_surface,
    wl_resource* positioner)
{
    std::experimental::optional<WlSurfaceRole*> parent_role;
    if (parent_surface)
    {
        XdgSurfaceStable* parent_xdg_surface = XdgSurfaceStable::from(parent_surface.value());
        std::experimental::optional<WindowWlSurfaceRole*> parent_window_role = parent_xdg_surface->window_role();
        if (parent_window_role)
            parent_role = static_cast<WlSurfaceRole*>(parent_window_role.value());
        else
            log_warning("Parent window of a popup has no role");
    }

    auto popup = new XdgPopupStable{new_popup, this, parent_role, positioner, surface};
    set_window_role(popup);
}

void mf::XdgSurfaceStable::set_window_geometry(int32_t x, int32_t y, int32_t width, int32_t height)
{
    if (auto& role = window_role())
    {
        role.value()->set_pending_offset(geom::Displacement{-x, -y});
        role.value()->set_pending_width(geom::Width{width});
        role.value()->set_pending_height(geom::Height{height});
    }
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

mf::XdgPopupStable::XdgPopupStable(
    wl_resource* new_resource,
    XdgSurfaceStable* xdg_surface,
    std::experimental::optional<WlSurfaceRole*> parent_role,
    struct wl_resource* positioner,
    WlSurface* surface)
    : wayland::XdgPopup(new_resource, Version<1>()),
      WindowWlSurfaceRole(
          &xdg_surface->xdg_shell.seat,
          mw::XdgPopup::client,
          surface,
          xdg_surface->xdg_shell.shell,
          xdg_surface->xdg_shell.output_manager),
      xdg_surface{xdg_surface}
{
    auto specification = static_cast<mir::shell::SurfaceSpecification*>(
                                static_cast<XdgPositionerStable*>(
                                    static_cast<wayland::XdgPositioner*>(
                                        wl_resource_get_user_data(positioner))));

    specification->type = mir_window_type_freestyle;
    specification->placement_hints = mir_placement_hints_slide_any;
    if (parent_role)
        specification->parent_id = parent_role.value()->surface_id();

    apply_spec(*specification);
}

void mf::XdgPopupStable::grab(struct wl_resource* seat, uint32_t serial)
{
    (void)seat, (void)serial;
    // TODO
}

void mf::XdgPopupStable::destroy()
{
    destroy_wayland_object();
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

void mf::XdgPopupStable::handle_close_request()
{
    send_popup_done_event();
}

auto mf::XdgPopupStable::from(wl_resource* resource) -> XdgPopupStable*
{
    auto popup = dynamic_cast<XdgPopupStable*>(wayland::XdgPopup::from(resource));
    if (!popup)
        BOOST_THROW_EXCEPTION(std::runtime_error("Invalid resource given to XdgPopupStable::from()"));
    return popup;
}

// XdgToplevelStable

mf::XdgToplevelStable::XdgToplevelStable(wl_resource* new_resource, XdgSurfaceStable* xdg_surface, WlSurface* surface)
    : mw::XdgToplevel(new_resource, Version<1>()),
      WindowWlSurfaceRole(
          &xdg_surface->xdg_shell.seat,
          wayland::XdgToplevel::client,
          surface,
          xdg_surface->xdg_shell.shell,
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
    destroy_wayland_object();
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

void mf::XdgToplevelStable::set_app_id(std::string const& app_id)
{
    WindowWlSurfaceRole::set_application_id(app_id);
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
    case ResizeEdge::top:
        edge = mir_resize_edge_north;
        break;

    case ResizeEdge::bottom:
        edge = mir_resize_edge_south;
        break;

    case ResizeEdge::left:
        edge = mir_resize_edge_west;
        break;

    case ResizeEdge::right:
        edge = mir_resize_edge_east;
        break;

    case ResizeEdge::top_left:
        edge = mir_resize_edge_northwest;
        break;

    case ResizeEdge::bottom_left:
        edge = mir_resize_edge_southwest;
        break;

    case ResizeEdge::top_right:
        edge = mir_resize_edge_northeast;
        break;

    case ResizeEdge::bottom_right:
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

void mf::XdgToplevelStable::handle_state_change(MirWindowState /*new_state*/)
{
    send_toplevel_configure();
}

void mf::XdgToplevelStable::handle_active_change(bool /*is_now_active*/)
{
    send_toplevel_configure();
}

void mf::XdgToplevelStable::handle_resize(std::experimental::optional<geometry::Point> const& /*new_top_left*/,
                                          geometry::Size const& /*new_size*/)
{
    send_toplevel_configure();
}

void mf::XdgToplevelStable::handle_close_request()
{
    send_close_event();
}

void mf::XdgToplevelStable::send_toplevel_configure()
{
    wl_array states;
    wl_array_init(&states);

    if (is_active())
    {
        if (uint32_t *state = static_cast<decltype(state)>(wl_array_add(&states, sizeof *state)))
            *state = State::activated;
    }

    switch (window_state())
    {
    case mir_window_state_maximized:
    case mir_window_state_horizmaximized:
    case mir_window_state_vertmaximized:
        if (uint32_t *state = static_cast<decltype(state)>(wl_array_add(&states, sizeof *state)))
            *state = State::maximized;
        break;

    case mir_window_state_fullscreen:
        if (uint32_t *state = static_cast<decltype(state)>(wl_array_add(&states, sizeof *state)))
            *state = State::fullscreen;
        break;

    default:
        break;
    }

    // 0 sizes means default for toplevel configure
    geom::Size size = requested_window_size().value_or(geom::Size{0, 0});

    send_configure_event(size.width.as_int(), size.height.as_int(), &states);
    wl_array_release(&states);

    xdg_surface->send_configure();
}

mf::XdgToplevelStable* mf::XdgToplevelStable::from(wl_resource* surface)
{
    auto* tmp = wl_resource_get_user_data(surface);
    return static_cast<XdgToplevelStable*>(static_cast<wayland::XdgToplevel*>(tmp));
}

// XdgPositionerStable

mf::XdgPositionerStable::XdgPositionerStable(wl_resource* new_resource)
    : mw::XdgPositioner(new_resource, Version<1>())
{
    // specifying gravity is not required by the xdg shell protocol, but is by Mir window managers
    surface_placement_gravity = mir_placement_gravity_center;
    aux_rect_placement_gravity = mir_placement_gravity_center;
}

void mf::XdgPositionerStable::destroy()
{
    destroy_wayland_object();
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
        case Anchor::top:
            placement = mir_placement_gravity_north;
            break;

        case Anchor::bottom:
            placement = mir_placement_gravity_south;
            break;

        case Anchor::left:
            placement = mir_placement_gravity_west;
            break;

        case Anchor::right:
            placement = mir_placement_gravity_east;
            break;

        case Anchor::top_left:
            placement = mir_placement_gravity_northwest;
            break;

        case Anchor::bottom_left:
            placement = mir_placement_gravity_southwest;
            break;

        case Anchor::top_right:
            placement = mir_placement_gravity_northeast;
            break;

        case Anchor::bottom_right:
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
        case Gravity::top:
            placement = mir_placement_gravity_south;
            break;

        case Gravity::bottom:
            placement = mir_placement_gravity_north;
            break;

        case Gravity::left:
            placement = mir_placement_gravity_east;
            break;

        case Gravity::right:
            placement = mir_placement_gravity_west;
            break;

        case Gravity::top_left:
            placement = mir_placement_gravity_southeast;
            break;

        case Gravity::bottom_left:
            placement = mir_placement_gravity_northeast;
            break;

        case Gravity::top_right:
            placement = mir_placement_gravity_southwest;
            break;

        case Gravity::bottom_right:
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

auto mf::XdgShellStable::get_window(wl_resource* surface) -> std::shared_ptr<Surface>
{
    namespace mw = mir::wayland;

    if (mw::XdgSurface::is_instance(surface))
    {
        auto const xdgsurface = XdgSurfaceStable::from(surface);
        if (auto const role = xdgsurface->window_role())
        {
            auto const id = role.value()->surface_id();
            if (id.as_value())
            {
                auto const session = get_session(xdgsurface->client);
                return session->get_surface(id);
            }
        }

        log_debug("No window currently associated with wayland::XdgSurface %p", surface);
    }

    return {};
}
