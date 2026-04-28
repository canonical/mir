/*
 * Copyright © Canonical Ltd.
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
 */

#include "xdg_shell_stable.h"

#include "wl_surface.h"
#include "wayland_utils.h"
#include "protocol_error.h"
#include "client.h"

#include <mir/frontend/wayland.h>
#include <mir/shell/surface_specification.h>
#include <mir/shell/shell.h>
#include <mir/scene/surface.h>
#include <mir/log.h>

#include <boost/throw_exception.hpp>

namespace mf = mir::frontend;
namespace ms = mir::scene;
namespace msh = mir::shell;
namespace geom = mir::geometry;
namespace mw = mir::wayland_rs;

namespace mir
{
namespace frontend
{

class XdgSurfaceStable : public mw::XdgSurfaceImpl, public std::enable_shared_from_this<XdgSurfaceStable>
{
public:
    static XdgSurfaceStable* from(mw::XdgSurfaceImpl* impl);

    XdgSurfaceStable(std::shared_ptr<wayland_rs::Client> const& client, WlSurface* surface, XdgShellStable const& xdg_shell);
    ~XdgSurfaceStable() = default;

    auto get_toplevel() -> std::shared_ptr<wayland_rs::XdgToplevelImpl> override;
    auto get_popup(wayland_rs::Weak<XdgSurfaceImpl> const& parent, bool has_parent, wayland_rs::Weak<wayland_rs::XdgPositionerImpl> const& positioner)
        -> std::shared_ptr<wayland_rs::XdgPopupImpl> override;
    void set_window_geometry(int32_t x, int32_t y, int32_t width, int32_t height) override;
    void ack_configure(uint32_t serial) override;

    void send_configure();

    mw::Weak<WindowWlSurfaceRole> const& window_role();

private:
    std::shared_ptr<wayland_rs::Client> client;
    mw::Weak<WindowWlSurfaceRole> window_role_;
    mw::Weak<WlSurface> const surface;

public:
    XdgShellStable const& xdg_shell;
};

class XdgPositionerStable : public mw::XdgPositionerImpl, public shell::SurfaceSpecification
{
public:
    XdgPositionerStable();

    void ensure_complete() const;

    bool reactive{false};

    void set_size(int32_t width, int32_t height) override;
    void set_anchor_rect(int32_t x, int32_t y, int32_t width, int32_t height) override;
    void set_anchor(uint32_t anchor) override;
    void set_gravity(uint32_t gravity) override;
    void set_constraint_adjustment(uint32_t constraint_adjustment) override;
    void set_offset(int32_t x, int32_t y) override;
    void set_reactive() override;
    void set_parent_size(int32_t parent_width, int32_t parent_height) override;
    void set_parent_configure(uint32_t serial) override;
};

}
}
namespace mf = mir::frontend;  // Keep CLion's parsing happy

auto mf::XdgShellStable::create_positioner() -> std::shared_ptr<wayland_rs::XdgPositionerImpl>
{
    return std::make_shared<XdgPositionerStable>();
}

auto mf::XdgShellStable::get_xdg_surface(wayland_rs::Weak<wayland_rs::WlSurfaceImpl> const& surface) -> std::shared_ptr<wayland_rs::XdgSurfaceImpl>
{
    return std::make_shared<XdgSurfaceStable>(client, WlSurface::from(&surface.value()), *this);
}

void mf::XdgShellStable::pong(uint32_t serial)
{
    (void)serial;
    // TODO
}

// XdgSurfaceStable

mf::XdgSurfaceStable* mf::XdgSurfaceStable::from(mw::XdgSurfaceImpl* impl)
{
    return static_cast<XdgSurfaceStable*>(impl);
}

mf::XdgSurfaceStable::XdgSurfaceStable(std::shared_ptr<wayland_rs::Client> const& client, WlSurface* surface, XdgShellStable const& xdg_shell)
    : client{client},
      surface{surface->shared_from_this()},
      xdg_shell{xdg_shell}
{
}

auto mf::XdgSurfaceStable::get_toplevel() -> std::shared_ptr<wayland_rs::XdgToplevelImpl>
{
    if (!surface)
    {
        BOOST_THROW_EXCEPTION(mw::ProtocolError(
            object_id(),
            mw::generic_error_code,
            "Tried to create toplevel after destroying surface"));
    }
    if (window_role_)
    {
        BOOST_THROW_EXCEPTION(mw::ProtocolError(
            object_id(),
            Error::already_constructed,
            "Tried to create toplevel on surface with existing role"));
    }
    auto toplevel = std::make_shared<XdgToplevelStable>(client, this, &surface.value());
    window_role_ = mw::Weak<WindowWlSurfaceRole>(toplevel);
    return toplevel;
}

auto mf::XdgSurfaceStable::get_popup(wayland_rs::Weak<XdgSurfaceImpl> const& parent, bool has_parent, wayland_rs::Weak<wayland_rs::XdgPositionerImpl> const& positioner)
    -> std::shared_ptr<wayland_rs::XdgPopupImpl>
{
    std::optional<WlSurfaceRole*> parent_role;
    if (has_parent)
    {
        XdgSurfaceStable* parent_xdg_surface = XdgSurfaceStable::from(&parent.value());
        auto const& parent_window_role = parent_xdg_surface->window_role();
        if (parent_window_role)
        {
            parent_role = static_cast<WlSurfaceRole*>(&parent_window_role.value());
        }
        else
        {
            log_warning("Parent window of a popup has no role");
        }
    }

    auto xdg_positioner = static_cast<XdgPositionerStable*>(&positioner.value());

    if (!surface)
    {
        BOOST_THROW_EXCEPTION(mw::ProtocolError(
            object_id(),
            mw::generic_error_code,
            "Tried to create popup after destroying surface"));
    }
    if (window_role_)
    {
        BOOST_THROW_EXCEPTION(mw::ProtocolError(
            object_id(),
            Error::already_constructed,
            "Tried to create popup on surface with existing role"));
    }

    auto popup = std::make_shared<XdgPopupStable>(client, this, parent_role, *xdg_positioner, &surface.value());
    window_role_ = mw::Weak<WindowWlSurfaceRole>(popup);
    return popup;
}

void mf::XdgSurfaceStable::set_window_geometry(int32_t x, int32_t y, int32_t width, int32_t height)
{
    if (width <= 0 || height <= 0)
    {
        BOOST_THROW_EXCEPTION(mw::ProtocolError(
            object_id(),
            mw::generic_error_code,
            "Invalid %s size %dx%d", "xdg_surface_stable", width, height));
    }
    if (window_role_)
    {
        window_role_.value().set_pending_offset(geom::Displacement{-x, -y});
        window_role_.value().set_pending_width(geom::Width{width});
        window_role_.value().set_pending_height(geom::Height{height});
    }
}

void mf::XdgSurfaceStable::ack_configure(uint32_t serial)
{
    (void)serial;
    // TODO
}

void mf::XdgSurfaceStable::send_configure()
{
    auto const serial = client->next_serial(nullptr);
    send_configure_event(serial);
}

mw::Weak<mf::WindowWlSurfaceRole> const& mf::XdgSurfaceStable::window_role()
{
    return window_role_;
}

// XdgPopupStable

mf::XdgPopupStable::XdgPopupStable(
    std::shared_ptr<mw::Client> const& client,
    XdgSurfaceStable* xdg_surface,
    std::optional<WlSurfaceRole*> parent_role,
    XdgPositionerStable& positioner,
    WlSurface* surface)
    : WindowWlSurfaceRole(
          xdg_surface->xdg_shell.wayland_executor,
          &xdg_surface->xdg_shell.seat,
          client.get(),
          surface->shared_from_this(),
          xdg_surface->xdg_shell.shell,
          xdg_surface->xdg_shell.output_manager,
          xdg_surface->xdg_shell.surface_registry),
      reactive{positioner.reactive},
      aux_rect{positioner.aux_rect ? positioner.aux_rect.value() : geom::Rectangle{}},
      shell{xdg_surface->xdg_shell.shell},
      xdg_surface{xdg_surface->shared_from_this()}
{
    positioner.ensure_complete();
    positioner.type = mir_window_type_tip;
    if (parent_role)
    {
        if (auto scene_surface = parent_role.value()->scene_surface())
        {
            positioner.parent = scene_surface.value();
        }
    }

    apply_spec(positioner);
}

void mf::XdgPopupStable::set_aux_rect_offset_now(geom::Displacement const& new_aux_rect_offset)
{
    if (aux_rect_offset == new_aux_rect_offset)
    {
        return;
    }

    aux_rect_offset = new_aux_rect_offset;

    shell::SurfaceSpecification spec;
    spec.aux_rect = aux_rect;
    spec.aux_rect.value().top_left += aux_rect_offset;

    auto const scene_surface_{scene_surface()};
    if (scene_surface_)
    {
        if (auto const session = scene_surface_.value()->session().lock())
        {
            shell->modify_surface(session, scene_surface_.value(), spec);
        }
    }
    else
    {
        apply_spec(spec);
    }
}

auto mf::XdgPopupStable::grab(wayland_rs::Weak<wayland_rs::WlSeatImpl> const& seat, uint32_t serial) -> void
{
    (void)seat, (void)serial;
    set_type(mir_window_type_menu);
}

auto mf::XdgPopupStable::reposition(wayland_rs::Weak<wayland_rs::XdgPositionerImpl> const& positioner_impl, uint32_t token) -> void
{
    auto const& positioner = *static_cast<XdgPositionerStable*>(&positioner_impl.value());
    positioner.ensure_complete();

    reactive = positioner.reactive;
    aux_rect = positioner.aux_rect ? positioner.aux_rect.value() : geom::Rectangle{};
    reposition_token = token;

    auto const scene_surface_{scene_surface()};
    if (scene_surface_)
    {
        if (auto const session = scene_surface_.value()->session().lock())
        {
            shell->modify_surface(session, scene_surface_.value(), positioner);
        }
    }
    else
    {
        apply_spec(positioner);
    }
}

void mf::XdgPopupStable::handle_resize(
    std::optional<geometry::Point> const& new_top_left,
    geometry::Size const& new_size)
{
    auto const prev_rect = popup_rect();
    if (new_top_left)
    {
        cached_top_left = new_top_left.value() - aux_rect_offset;
    }
    cached_size = new_size;
    auto const new_rect = popup_rect();

    bool const configure_appropriate =
        reposition_token ||
        initial_configure_pending ||
        (reactive && new_rect != prev_rect);
    if (new_rect && configure_appropriate)
    {
        if (reposition_token)
        {
            send_repositioned_event(reposition_token.value());
            reposition_token = std::nullopt;
        }
        send_configure_event(cached_top_left.value().x.as_int(),
                             cached_top_left.value().y.as_int(),
                             cached_size.value().width.as_int(),
                             cached_size.value().height.as_int());
        if (xdg_surface) xdg_surface.value().send_configure();
        initial_configure_pending = false;
    }
}

void mf::XdgPopupStable::handle_close_request()
{
    send_popup_done_event();
}

auto mf::XdgPopupStable::from(wayland_rs::XdgPopupImpl* impl) -> XdgPopupStable*
{
    auto popup = dynamic_cast<XdgPopupStable*>(impl);
    if (!popup)
        BOOST_THROW_EXCEPTION(std::runtime_error("Invalid resource given to XdgPopupStable::from()"));
    return popup;
}

auto mf::XdgPopupStable::popup_rect() const -> std::optional<geometry::Rectangle>
{
    if (cached_top_left && cached_size)
    {
        return geometry::Rectangle{cached_top_left.value(), cached_size.value()};
    }
    else
    {
        return std::nullopt;
    }
}

void mf::XdgPopupStable::destroy_role() const
{
    // TODO:
    // destroy_and_delete();
    // wl_resource_destroy(resource);
}

// XdgToplevelStable

mf::XdgToplevelStable::XdgToplevelStable(
    std::shared_ptr<wayland_rs::Client> const& client,
    XdgSurfaceStable* xdg_surface,
    WlSurface* surface)
    : WindowWlSurfaceRole(
          xdg_surface->xdg_shell.wayland_executor,
          &xdg_surface->xdg_shell.seat,
          client.get(),
          surface->shared_from_this(),
          xdg_surface->xdg_shell.shell,
          xdg_surface->xdg_shell.output_manager,
          xdg_surface->xdg_shell.surface_registry),
      xdg_surface{xdg_surface->shared_from_this()}
{
    std::vector<uint8_t> capabilities{
        WmCapabilities::maximize,
        WmCapabilities::minimize,
        WmCapabilities::fullscreen,
    };
    send_wm_capabilities_event(capabilities);

     std::vector<uint8_t> states{};
    send_configure_event(0, 0, states);
    xdg_surface->send_configure();
}

auto mf::XdgToplevelStable::set_parent(wayland_rs::Weak<XdgToplevelImpl> const& parent, bool has_parent) -> void
{
    if (has_parent)
    {
        WindowWlSurfaceRole::set_parent(XdgToplevelStable::from(&parent.value())->scene_surface());
    }
    else
    {
        WindowWlSurfaceRole::set_parent({});
    }
}


void mf::XdgToplevelStable::set_title(rust::String title)
{
    WindowWlSurfaceRole::set_title(title.c_str());
}

void mf::XdgToplevelStable::set_app_id(rust::String app_id)
{
    WindowWlSurfaceRole::set_application_id(app_id.c_str());
}

auto mf::XdgToplevelStable::show_window_menu(wayland_rs::Weak<wayland_rs::WlSeatImpl> const& seat, uint32_t serial, int32_t x, int32_t y) -> void
{
    (void)seat, (void)serial, (void)x, (void)y;
    // TODO
}

auto mf::XdgToplevelStable::r_move(wayland_rs::Weak<wayland_rs::WlSeatImpl> const&, uint32_t serial) -> void
{
    initiate_interactive_move(serial);
}

void mf::XdgToplevelStable::resize(wayland_rs::Weak<wayland_rs::WlSeatImpl> const&, uint32_t serial, uint32_t edges)
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

    case ResizeEdge::none:
        edge = mir_resize_edge_none;
        break;

    default:
        BOOST_THROW_EXCEPTION(mw::ProtocolError(
            object_id(),
            Error::invalid_resize_edge,
            "Invalid resize edge %d", edges));
    }

    initiate_interactive_resize(edge, serial);
}

void mf::XdgToplevelStable::set_max_size(int32_t width, int32_t height)
{
    if (width < 0 || height < 0)
    {
        BOOST_THROW_EXCEPTION(mw::ProtocolError(
            object_id(),
            Error::invalid_size,
            "Invalid maximum size %dx%d", width, height));
    }
    WindowWlSurfaceRole::set_max_size(width, height);
}

void mf::XdgToplevelStable::set_min_size(int32_t width, int32_t height)
{
    if (width < 0 || height < 0)
    {
        BOOST_THROW_EXCEPTION(mw::ProtocolError(
            object_id(),
            Error::invalid_size,
            "Invalid minimum size %dx%d", width, height));
    }
    WindowWlSurfaceRole::set_min_size(width, height);
}

void mf::XdgToplevelStable::set_maximized()
{
    // We must process this request immediately (i.e. don't defer until commit())
    add_state_now(mir_window_state_maximized);
}

void mf::XdgToplevelStable::unset_maximized()
{
    // We must process this request immediately (i.e. don't defer until commit())
    remove_state_now(mir_window_state_maximized);
}

auto mf::XdgToplevelStable::set_fullscreen(wayland_rs::Weak<wayland_rs::WlOutputImpl> const& output, bool) -> void
{
    WindowWlSurfaceRole::set_fullscreen(output);
}

void mf::XdgToplevelStable::unset_fullscreen()
{
    // We must process this request immediately (i.e. don't defer until commit())
    // TODO: should we instead restore the previous state?
    remove_state_now(mir_window_state_fullscreen);
}

void mf::XdgToplevelStable::set_minimized()
{
    // We must process this request immediately (i.e. don't defer until commit())
    add_state_now(mir_window_state_minimized);
}

void mf::XdgToplevelStable::handle_state_change(MirWindowState /*new_state*/)
{
    send_toplevel_configure();
}

void mf::XdgToplevelStable::handle_active_change(bool /*is_now_active*/)
{
    send_toplevel_configure();
}

void mf::XdgToplevelStable::handle_resize(
    std::optional<geometry::Point> const& /*new_top_left*/,
    geometry::Size const& /*new_size*/)
{
    send_toplevel_configure();
}

void mf::XdgToplevelStable::handle_tiled_edges(Flags<MirTiledEdge> /*tiled_edges*/)
{
    send_toplevel_configure();
}

void mf::XdgToplevelStable::handle_close_request()
{
    send_close_event();
}

void mf::XdgToplevelStable::send_toplevel_configure()
{
    std::vector<uint8_t> states;

    if (is_active())
    {
        states.push_back(State::activated);
    }

    auto opt_window_size = requested_window_size();

    if (auto const surface = scene_surface())
    {
        auto const s = surface.value();
        auto const state = s->state_tracker();

        if (state.has_any({
            mir_window_state_maximized,
            mir_window_state_horizmaximized,
            mir_window_state_vertmaximized}))
        {
            states.push_back(State::maximized);
        }

        if (state.has(mir_window_state_fullscreen))
        {
            states.push_back(State::fullscreen);
        }

        auto tiled_edges = s->tiled_edges();
        if (tiled_edges & mir_tiled_edge_north)
        {
            states.push_back(State::tiled_top);
        }
        if (tiled_edges & mir_tiled_edge_east)
        {
            states.push_back(State::tiled_right);
        }
        if (tiled_edges & mir_tiled_edge_south)
        {
            states.push_back(State::tiled_bottom);
        }
        if (tiled_edges & mir_tiled_edge_west)
        {
            states.push_back(State::tiled_left);
        }

        if (state.has_any({
            mir_window_state_maximized,
            mir_window_state_horizmaximized,
            mir_window_state_vertmaximized,
            mir_window_state_fullscreen}) ||
                tiled_edges != mir_tiled_edge_none)
        {
            opt_window_size = s->content_size();
        }

        // TODO: plumb resizing state through Mir?
    }

    // 0 sizes means default for toplevel configure
    auto size = opt_window_size.value_or(geom::Size{0, 0});

    send_configure_event(size.width.as_int(), size.height.as_int(), states);

    if (xdg_surface) xdg_surface.value().send_configure();
}

mf::XdgToplevelStable* mf::XdgToplevelStable::from(mw::XdgToplevelImpl* impl)
{
    return static_cast<XdgToplevelStable*>(impl);
}

void mf::XdgToplevelStable::destroy_role() const
{
    // TODO:
    // wl_resource_destroy(resource);
}

// XdgPositionerStable

mf::XdgPositionerStable::XdgPositionerStable()
{
    // specifying gravity is not required by the xdg shell protocol, but is by Mir window managers
    surface_placement_gravity = mir_placement_gravity_center;
    aux_rect_placement_gravity = mir_placement_gravity_center;
    placement_hints = static_cast<MirPlacementHints>(0);
}

void mf::XdgPositionerStable::ensure_complete() const
{
    if (!width || !height || !aux_rect)
    {
        BOOST_THROW_EXCEPTION(mw::ProtocolError(
            object_id(),
            mw::XdgWmBaseImpl::Error::invalid_positioner,
            "Incomplete positioner"));
    }
}

void mf::XdgPositionerStable::set_size(int32_t width, int32_t height)
{
    if (width <= 0 || height <= 0)
    {
        BOOST_THROW_EXCEPTION(mw::ProtocolError(
            object_id(),
            mw::XdgPositionerImpl::Error::invalid_input,
            "Invalid popup positioner size: %dx%d", width, height));
    }
    this->width = geom::Width{width};
    this->height = geom::Height{height};
}

void mf::XdgPositionerStable::set_anchor_rect(int32_t x, int32_t y, int32_t width, int32_t height)
{
    if (width < 0 || height < 0)
    {
        BOOST_THROW_EXCEPTION(mw::ProtocolError(
            object_id(),
            mw::XdgPositionerImpl::Error::invalid_input,
            "Invalid popup anchor rect size: %dx%d", width, height));
    }
    aux_rect = geom::Rectangle{{x, y}, {width, height}};
}

void mf::XdgPositionerStable::set_anchor(uint32_t anchor)
{
    MirPlacementGravity placement = mir_placement_gravity_center;

    switch (anchor)
    {
        case Anchor::none:
            placement = mir_placement_gravity_center;
            break;

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
            BOOST_THROW_EXCEPTION(mw::ProtocolError(
                object_id(),
                mw::XdgPositionerImpl::Error::invalid_input,
                "Invalid anchor value %u", anchor));
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

        case Gravity::none:
            placement = mir_placement_gravity_center;
            break;

        default:
            BOOST_THROW_EXCEPTION(mw::ProtocolError(
                object_id(),
                mw::XdgPositionerImpl::Error::invalid_input,
                "Invalid gravity value %d", gravity));
    }

    surface_placement_gravity = placement;
}

void mf::XdgPositionerStable::set_constraint_adjustment(uint32_t constraint_adjustment)
{
    uint32_t new_placement_hints{0};
    if (constraint_adjustment & ConstraintAdjustment::slide_x)
    {
        new_placement_hints |= mir_placement_hints_slide_x;
    }
    if (constraint_adjustment & ConstraintAdjustment::slide_y)
    {
        new_placement_hints |= mir_placement_hints_slide_y;
    }
    if (constraint_adjustment & ConstraintAdjustment::flip_x)
    {
        new_placement_hints |= mir_placement_hints_flip_x;
    }
    if (constraint_adjustment & ConstraintAdjustment::flip_y)
    {
        new_placement_hints |= mir_placement_hints_flip_y;
    }
    if (constraint_adjustment & ConstraintAdjustment::resize_x)
    {
        new_placement_hints |= mir_placement_hints_resize_x;
    }
    if (constraint_adjustment & ConstraintAdjustment::resize_y)
    {
        new_placement_hints |= mir_placement_hints_resize_y;
    }
    placement_hints = static_cast<MirPlacementHints>(new_placement_hints);
}

void mf::XdgPositionerStable::set_offset(int32_t x, int32_t y)
{
    aux_rect_placement_offset_x = x;
    aux_rect_placement_offset_y = y;
}

void mf::XdgPositionerStable::set_reactive()
{
    reactive = true;
}

void mf::XdgPositionerStable::set_parent_size(int32_t parent_width, int32_t parent_height)
{
    (void)parent_width;
    (void)parent_height;
    // TODO
    log_warning("xdg_positioner.set_parent_size not implemented");
}

void mf::XdgPositionerStable::set_parent_configure(uint32_t serial)
{
    (void)serial;
    // TODO
    log_warning("xdg_positioner.set_parent_configure not implemented");
}

auto mf::XdgShellStable::get_window(wayland_rs::XdgSurfaceImpl* surface) -> std::shared_ptr<scene::Surface>
{
    namespace mw = mir::wayland;

    auto const xdgsurface = XdgSurfaceStable::from(surface);
    if (auto const& role = xdgsurface->window_role())
    {
        if (auto const scene_surface = role.value().scene_surface())
        {
            return scene_surface.value();
        }
    }

    log_debug("No window currently associated with wayland::XdgSurface %p", static_cast<void*>(surface));

    return {};
}
