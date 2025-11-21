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

#include "mir/wayland/protocol_error.h"
#include "mir/wayland/client.h"
#include "mir/frontend/wayland.h"
#include "mir/shell/surface_specification.h"
#include "mir/shell/shell.h"
#include "mir/scene/surface.h"
#include "mir/log.h"

#include <boost/throw_exception.hpp>

namespace mf = mir::frontend;
namespace ms = mir::scene;
namespace msh = mir::shell;
namespace geom = mir::geometry;
namespace mw = mir::wayland;

namespace mir
{
namespace frontend
{

class XdgSurfaceStable : public mw::XdgSurface
{
public:
    static XdgSurfaceStable* from(wl_resource* surface);

    XdgSurfaceStable(wl_resource* new_resource, WlSurface* surface, XdgShellStable const& xdg_shell);
    ~XdgSurfaceStable() = default;

    void get_toplevel(wl_resource* new_toplevel) override;
    void get_popup(
        wl_resource* new_popup,
        std::optional<wl_resource*> const& parent_surface,
        wl_resource* positioner) override;
    void set_window_geometry(int32_t x, int32_t y, int32_t width, int32_t height) override;
    void ack_configure(uint32_t serial) override;

    void send_configure();

    mw::Weak<WindowWlSurfaceRole> const& window_role();

    using mw::XdgSurface::client;
    using mw::XdgSurface::resource;

private:
    void set_window_role(WindowWlSurfaceRole* role);

    mw::Weak<WindowWlSurfaceRole> window_role_;
    mw::Weak<WlSurface> const surface;

public:
    XdgShellStable const& xdg_shell;
};

class XdgPositionerStable : public mw::XdgPositioner, public shell::SurfaceSpecification
{
public:
    XdgPositionerStable(wl_resource* new_resource);

    void ensure_complete() const;

    bool reactive{false};

private:
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

class mf::XdgShellStable::Instance : public mw::XdgWmBase
{
public:
    Instance(wl_resource* new_resource, mf::XdgShellStable* shell);

private:
    void create_positioner(wl_resource* new_positioner) override;
    void get_xdg_surface(wl_resource* new_xdg_surface, wl_resource* surface) override;
    void pong(uint32_t serial) override;

    mf::XdgShellStable* const shell;
};

// XdgShellStable

mf::XdgShellStable::XdgShellStable(
    struct wl_display* display,
    Executor& wayland_executor,
    std::shared_ptr<msh::Shell> const& shell,
    WlSeat& seat,
    OutputManager* output_manager)
    : Global(display, Version<5>()),
      wayland_executor{wayland_executor},
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
    : XdgWmBase{new_resource, Version<5>()},
      shell{shell}
{
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
    return static_cast<XdgSurfaceStable*>(static_cast<mw::XdgSurface*>(tmp));
}

mf::XdgSurfaceStable::XdgSurfaceStable(wl_resource* new_resource, WlSurface* surface, XdgShellStable const& xdg_shell)
    : mw::XdgSurface(new_resource, Version<5>()),
      surface{surface},
      xdg_shell{xdg_shell}
{
}

void mf::XdgSurfaceStable::get_toplevel(wl_resource* new_toplevel)
{
    if (!surface)
    {
        BOOST_THROW_EXCEPTION(mw::ProtocolError(
            resource,
            mw::generic_error_code,
            "Tried to create toplevel after destroying surface"));
    }
    auto toplevel = new XdgToplevelStable{new_toplevel, this, &surface.value()};
    set_window_role(toplevel);
}

void mf::XdgSurfaceStable::get_popup(
    wl_resource* new_popup,
    std::optional<struct wl_resource*> const& parent_surface,
    wl_resource* positioner)
{
    std::optional<WlSurfaceRole*> parent_role;
    if (parent_surface)
    {
        XdgSurfaceStable* parent_xdg_surface = XdgSurfaceStable::from(parent_surface.value());
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

    auto xdg_positioner = static_cast<XdgPositionerStable*>(
        static_cast<mw::XdgPositioner*>(
            wl_resource_get_user_data(positioner)));

    if (!surface)
    {
        BOOST_THROW_EXCEPTION(mw::ProtocolError(
            resource,
            mw::generic_error_code,
            "Tried to create popup after destroying surface"));
    }

    auto popup = new XdgPopupStable{new_popup, this, parent_role, *xdg_positioner, &surface.value()};
    set_window_role(popup);
}

void mf::XdgSurfaceStable::set_window_geometry(int32_t x, int32_t y, int32_t width, int32_t height)
{
    if (width <= 0 || height <= 0)
    {
        BOOST_THROW_EXCEPTION(mw::ProtocolError(
            resource,
            mw::generic_error_code,
            "Invalid %s size %dx%d", interface_name, width, height));
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

void mf::XdgSurfaceStable::set_window_role(WindowWlSurfaceRole* role)
{
    if (window_role_)
    {
        log_warning("XdgSurfaceStable::window_role set multiple times");
    }

    window_role_ = mw::make_weak(role);
}

// XdgPopupStable

mf::XdgPopupStable::XdgPopupStable(
    wl_resource* new_resource,
    XdgSurfaceStable* xdg_surface,
    std::optional<WlSurfaceRole*> parent_role,
    XdgPositionerStable& positioner,
    WlSurface* surface)
    : mw::XdgPopup(new_resource, Version<5>()),
      WindowWlSurfaceRole(
          xdg_surface->xdg_shell.wayland_executor,
          &xdg_surface->xdg_shell.seat,
          mw::XdgPopup::client,
          surface,
          xdg_surface->xdg_shell.shell,
          xdg_surface->xdg_shell.output_manager),
      reactive{positioner.reactive},
      aux_rect{positioner.aux_rect ? positioner.aux_rect.value() : geom::Rectangle{}},
      shell{xdg_surface->xdg_shell.shell},
      xdg_surface{xdg_surface}
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
        shell->modify_surface(
            scene_surface_.value()->session().lock(),
            scene_surface_.value(),
            spec);
    }
    else
    {
        apply_spec(spec);
    }
}

void mf::XdgPopupStable::grab(struct wl_resource* seat, uint32_t serial)
{
    (void)seat, (void)serial;
    set_type(mir_window_type_menu);
}

void mf::XdgPopupStable::reposition(wl_resource* positioner_resource, uint32_t token)
{
    auto const& positioner = *static_cast<XdgPositionerStable*>(
        static_cast<mw::XdgPositioner*>(
            wl_resource_get_user_data(positioner_resource)));
    positioner.ensure_complete();

    reactive = positioner.reactive;
    aux_rect = positioner.aux_rect ? positioner.aux_rect.value() : geom::Rectangle{};
    reposition_token = token;

    auto const scene_surface_{scene_surface()};
    if (scene_surface_)
    {
        shell->modify_surface(
            scene_surface_.value()->session().lock(),
            scene_surface_.value(),
            positioner);
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

auto mf::XdgPopupStable::from(wl_resource* resource) -> XdgPopupStable*
{
    auto popup = dynamic_cast<XdgPopupStable*>(mw::XdgPopup::from(resource));
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
    wl_resource_destroy(resource);
}

// XdgToplevelStable

mf::XdgToplevelStable::XdgToplevelStable(wl_resource* new_resource, XdgSurfaceStable* xdg_surface, WlSurface* surface)
    : mw::XdgToplevel(new_resource, Version<5>()),
      WindowWlSurfaceRole(
          xdg_surface->xdg_shell.wayland_executor,
          &xdg_surface->xdg_shell.seat,
          mw::XdgToplevel::client,
          surface,
          xdg_surface->xdg_shell.shell,
          xdg_surface->xdg_shell.output_manager),
      xdg_surface{xdg_surface}
{
    if (version_supports_wm_capabilities())
    {
        std::vector<uint32_t> capabilities{
            WmCapabilities::maximize,
            WmCapabilities::minimize,
            WmCapabilities::fullscreen,
        };
        wl_array capability_array{};
        wl_array_init(&capability_array);
        for (auto& capability : capabilities)
        {
            if (uint32_t *item = static_cast<decltype(item)>(wl_array_add(&capability_array, sizeof *item)))
            {
                *item = capability;
            }
        }
        send_wm_capabilities_event(&capability_array);
        wl_array_release(&capability_array);
    }

    wl_array states{};
    wl_array_init(&states);
    send_configure_event(0, 0, &states);
    wl_array_release(&states);
    xdg_surface->send_configure();
}

void mf::XdgToplevelStable::set_parent(std::optional<struct wl_resource*> const& parent)
{
    if (parent && parent.value())
    {
        WindowWlSurfaceRole::set_parent(XdgToplevelStable::from(parent.value())->scene_surface());
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

void mf::XdgToplevelStable::move(struct wl_resource* /*seat*/, uint32_t serial)
{
    initiate_interactive_move(serial);
}

void mf::XdgToplevelStable::resize(struct wl_resource* /*seat*/, uint32_t serial, uint32_t edges)
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
            resource,
            Error::invalid_resize_edge,
            "Invalid resize edge %d", edges));
    }

    initiate_interactive_resize(edge, serial);
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
    add_state_now(mir_window_state_maximized);
}

void mf::XdgToplevelStable::unset_maximized()
{
    // We must process this request immediately (i.e. don't defer until commit())
    remove_state_now(mir_window_state_maximized);
}

void mf::XdgToplevelStable::set_fullscreen(std::optional<struct wl_resource*> const& output)
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
    wl_array states{};
    wl_array_init(&states);

    if (is_active())
    {
        if (uint32_t *state = static_cast<decltype(state)>(wl_array_add(&states, sizeof *state)))
            *state = State::activated;
    }

    if (auto const surface = scene_surface())
    {
        auto const state = surface.value()->state_tracker();
        if (state.has_any({
            mir_window_state_maximized,
            mir_window_state_horizmaximized,
            mir_window_state_vertmaximized}))
        {
            if (uint32_t *state = static_cast<decltype(state)>(wl_array_add(&states, sizeof *state)))
                *state = State::maximized;
        }

        if (state.has(mir_window_state_fullscreen))
        {
            if (uint32_t *state = static_cast<decltype(state)>(wl_array_add(&states, sizeof *state)))
                *state = State::fullscreen;
        }

        auto tiled_edges = surface.value()->tiled_edges();
        if (tiled_edges & mir_tiled_edge_north)
        {
            if (uint32_t *state = static_cast<decltype(state)>(wl_array_add(&states, sizeof *state)))
                *state = State::tiled_top;
        }
        if (tiled_edges & mir_tiled_edge_east)
        {
            if (uint32_t *state = static_cast<decltype(state)>(wl_array_add(&states, sizeof *state)))
                *state = State::tiled_right;
        }
        if (tiled_edges & mir_tiled_edge_south)
        {
            if (uint32_t *state = static_cast<decltype(state)>(wl_array_add(&states, sizeof *state)))
                *state = State::tiled_bottom;
        }
        if (tiled_edges & mir_tiled_edge_west)
        {
            if (uint32_t *state = static_cast<decltype(state)>(wl_array_add(&states, sizeof *state)))
                *state = State::tiled_left;
        }

        // TODO: plumb resizing state through Mir?
    }

    // 0 sizes means default for toplevel configure
    geom::Size size = requested_window_size().value_or(geom::Size{0, 0});

    send_configure_event(size.width.as_int(), size.height.as_int(), &states);
    wl_array_release(&states);

    if (xdg_surface) xdg_surface.value().send_configure();
}

mf::XdgToplevelStable* mf::XdgToplevelStable::from(wl_resource* surface)
{
    auto* tmp = wl_resource_get_user_data(surface);
    return static_cast<XdgToplevelStable*>(static_cast<mw::XdgToplevel*>(tmp));
}

void mf::XdgToplevelStable::destroy_role() const
{
    wl_resource_destroy(resource);
}

// XdgPositionerStable

mf::XdgPositionerStable::XdgPositionerStable(wl_resource* new_resource)
    : mw::XdgPositioner(new_resource, Version<5>())
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
            resource,
            mw::XdgWmBase::Error::invalid_positioner,
            "Incomplete positioner"));
    }
}

void mf::XdgPositionerStable::set_size(int32_t width, int32_t height)
{
    if (width <= 0 || height <= 0)
    {
        BOOST_THROW_EXCEPTION(mw::ProtocolError(
            resource,
            mw::XdgPositioner::Error::invalid_input,
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
            resource,
            mw::XdgPositioner::Error::invalid_input,
            "Invalid popup anchor rect size: %dx%d", width, height));
    }
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

        case Gravity::none:
            placement = mir_placement_gravity_center;
            break;

        default:
            BOOST_THROW_EXCEPTION(mw::ProtocolError(
                resource,
                mw::XdgPositioner::Error::invalid_input,
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
    if (constraint_adjustment & ConstraintAdjustment::flip_x)
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

auto mf::XdgShellStable::get_window(wl_resource* surface) -> std::shared_ptr<scene::Surface>
{
    namespace mw = mir::wayland;

    if (mw::XdgSurface::is_instance(surface))
    {
        auto const xdgsurface = XdgSurfaceStable::from(surface);
        if (auto const& role = xdgsurface->window_role())
        {
            if (auto const scene_surface = role.value().scene_surface())
            {
                return scene_surface.value();
            }
        }

        log_debug("No window currently associated with wayland::XdgSurface %p", static_cast<void*>(surface));
    }

    return {};
}
