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
#include "protocol_error.h"
#include "weak.h"

#include "wayland_rs/src/ffi.rs.h"

#include <mir/wayland/weak.h>
#include <mir/shell/surface_specification.h>
#include <mir/shell/shell.h>
#include <mir/scene/surface.h>
#include <mir/log.h>

#include <algorithm>
#include <iterator>
#include <stdexcept>
#include <vector>

namespace mf = mir::frontend;
namespace ms = mir::scene;
namespace msh = mir::shell;
namespace geom = mir::geometry;
namespace mw = mir::wayland;
namespace mwrs = mir::wayland_rs;

namespace mir
{
namespace frontend
{

class XdgSurfaceStable : public mwrs::XdgSurface
{
public:
    static auto from(mwrs::Weak<mwrs::XdgSurface> const& surface) -> XdgSurfaceStable*;

    XdgSurfaceStable(
        std::shared_ptr<mwrs::Client> client,
        rust::Box<mwrs::XdgSurfaceMiddleware> instance,
        uint32_t object_id,
        WlSurface* surface,
        XdgShellStable const& xdg_shell);
    ~XdgSurfaceStable() = default;

    auto get_toplevel(
        rust::Box<mwrs::XdgToplevelMiddleware> child_instance,
        uint32_t child_object_id) -> std::shared_ptr<mwrs::XdgToplevel> override;
    using mwrs::XdgSurface::get_popup;
    auto get_popup(
        std::optional<mwrs::Weak<mwrs::XdgSurface>> const& parent_surface,
        mwrs::Weak<mwrs::XdgPositioner> const& positioner,
        rust::Box<mwrs::XdgPopupMiddleware> child_instance,
        uint32_t child_object_id) -> std::shared_ptr<mwrs::XdgPopup> override;
    auto set_window_geometry(int32_t x, int32_t y, int32_t width, int32_t height) -> void override;
    auto ack_configure(uint32_t serial) -> void override;

    void send_configure();

    auto window_role() -> mw::Weak<WindowWlSurfaceRole> const&;

private:
    mw::Weak<WindowWlSurfaceRole> window_role_;
    mwrs::Weak<WlSurface> const surface;
    /// Serials of configure events sent on this xdg_surface that have not yet been
    /// consumed by an ack_configure, in the order they were sent. Acking a serial
    /// consumes it and every serial sent before it.
    std::vector<uint32_t> unacked_configure_serials;

public:
    XdgShellStable const& xdg_shell;
};

class XdgPositionerStable : public mwrs::XdgPositioner, public shell::SurfaceSpecification
{
public:
    XdgPositionerStable(
        std::shared_ptr<mwrs::Client> client,
        rust::Box<mwrs::XdgPositionerMiddleware> instance,
        uint32_t object_id);

    void ensure_complete();

    bool reactive{false};

private:
    auto set_size(int32_t width, int32_t height) -> void override;
    auto set_anchor_rect(int32_t x, int32_t y, int32_t width, int32_t height) -> void override;
    auto set_anchor(uint32_t anchor) -> void override;
    auto set_gravity(uint32_t gravity) -> void override;
    auto set_constraint_adjustment(uint32_t constraint_adjustment) -> void override;
    auto set_offset(int32_t x, int32_t y) -> void override;
    auto set_reactive() -> void override;
    auto set_parent_size(int32_t parent_width, int32_t parent_height) -> void override;
    auto set_parent_configure(uint32_t serial) -> void override;
};

}
}
namespace mf = mir::frontend;  // Keep CLion's parsing happy

namespace
{
// Wayland `array` arguments are opaque byte blobs on the wire. The xdg-shell
// `states` and `capabilities` arrays are defined as arrays of uint32_t enum
// values, so each entry must be appended as a uint32_t in native byte order
// (matching libwayland's historical `wl_array` semantics).
void append_u32(std::vector<uint8_t>& array, uint32_t value)
{
    auto const* const bytes = reinterpret_cast<uint8_t const*>(&value);
    array.insert(array.end(), bytes, bytes + sizeof(value));
}
}

// XdgShellStable

auto mf::create_xdg_shell_stable(
    std::shared_ptr<mwrs::Client> client,
    rust::Box<mwrs::XdgWmBaseMiddleware> instance,
    uint32_t object_id,
    Executor& wayland_executor,
    std::shared_ptr<msh::Shell> const& shell,
    WlSeat& seat,
    OutputManager* output_manager,
    std::shared_ptr<SurfaceRegistry> const& surface_registry)
-> std::shared_ptr<mwrs::XdgWmBase>
{
    return std::make_shared<XdgShellStable>(
        std::move(client),
        std::move(instance),
        object_id,
        wayland_executor,
        shell,
        seat,
        output_manager,
        surface_registry);
}

mf::XdgShellStable::XdgShellStable(
    std::shared_ptr<mwrs::Client> client,
    rust::Box<mwrs::XdgWmBaseMiddleware> instance,
    uint32_t object_id,
    Executor& wayland_executor,
    std::shared_ptr<msh::Shell> const& shell,
    WlSeat& seat,
    OutputManager* output_manager,
    std::shared_ptr<SurfaceRegistry> const& surface_registry)
    : XdgWmBase{std::move(client), std::move(instance), object_id},
      wayland_executor{wayland_executor},
      shell{shell},
      seat{seat},
      output_manager{output_manager},
      surface_registry{surface_registry}
{
}

auto mf::XdgShellStable::create_positioner(
    rust::Box<mwrs::XdgPositionerMiddleware> child_instance,
    uint32_t child_object_id) -> std::shared_ptr<mwrs::XdgPositioner>
{
    return std::make_shared<XdgPositionerStable>(client, std::move(child_instance), child_object_id);
}

auto mf::XdgShellStable::get_xdg_surface(
    mwrs::Weak<mwrs::Surface> const& surface,
    rust::Box<mwrs::XdgSurfaceMiddleware> child_instance,
    uint32_t child_object_id) -> std::shared_ptr<mwrs::XdgSurface>
{
    return std::make_shared<XdgSurfaceStable>(
        client,
        std::move(child_instance),
        child_object_id,
        mwrs::Surface::from<WlSurface>(surface),
        *this);
}

auto mf::XdgShellStable::pong(uint32_t serial) -> void
{
    (void)serial;
    // TODO
}

// XdgSurfaceStable

auto mf::XdgSurfaceStable::from(mwrs::Weak<mwrs::XdgSurface> const& surface) -> XdgSurfaceStable*
{
    return mwrs::XdgSurface::from<XdgSurfaceStable>(surface);
}

mf::XdgSurfaceStable::XdgSurfaceStable(
    std::shared_ptr<mwrs::Client> client,
    rust::Box<mwrs::XdgSurfaceMiddleware> instance,
    uint32_t object_id,
    WlSurface* surface,
    XdgShellStable const& xdg_shell)
    : mwrs::XdgSurface{std::move(client), std::move(instance), object_id},
      surface{surface},
      xdg_shell{xdg_shell}
{
}

auto mf::XdgSurfaceStable::get_toplevel(
    rust::Box<mwrs::XdgToplevelMiddleware> child_instance,
    uint32_t child_object_id) -> std::shared_ptr<mwrs::XdgToplevel>
{
    if (!surface)
    {
        throw mwrs::ProtocolError{
            object_id(),
            0,
            "Tried to create toplevel after destroying surface"};
    }
    if (window_role_)
    {
        throw mwrs::ProtocolError{
            object_id(),
            Error::already_constructed,
            "Tried to create toplevel on surface with existing role"};
    }
    auto toplevel = std::make_shared<XdgToplevelStable>(
        client, std::move(child_instance), child_object_id, this, &surface.value());
    window_role_ = mw::make_weak<WindowWlSurfaceRole>(toplevel.get());
    return toplevel;
}

auto mf::XdgSurfaceStable::get_popup(
    std::optional<mwrs::Weak<mwrs::XdgSurface>> const& parent_surface,
    mwrs::Weak<mwrs::XdgPositioner> const& positioner,
    rust::Box<mwrs::XdgPopupMiddleware> child_instance,
    uint32_t child_object_id) -> std::shared_ptr<mwrs::XdgPopup>
{
    std::optional<WlSurfaceRole*> parent_role;
    if (parent_surface)
    {
        XdgSurfaceStable* parent_xdg_surface = XdgSurfaceStable::from(parent_surface.value());
        if (parent_xdg_surface)
        {
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
    }

    auto* xdg_positioner = mwrs::XdgPositioner::from<XdgPositionerStable>(positioner);

    if (!surface)
    {
        throw mwrs::ProtocolError{
            object_id(),
            0,
            "Tried to create popup after destroying surface"};
    }
    if (window_role_)
    {
        throw mwrs::ProtocolError{
            object_id(),
            Error::already_constructed,
            "Tried to create popup on surface with existing role"};
    }

    auto popup = std::make_shared<XdgPopupStable>(
        client, std::move(child_instance), child_object_id, this, parent_role, *xdg_positioner, &surface.value());
    window_role_ = mw::make_weak<WindowWlSurfaceRole>(popup.get());
    return popup;
}

auto mf::XdgSurfaceStable::set_window_geometry(int32_t x, int32_t y, int32_t width, int32_t height) -> void
{
    if (width <= 0 || height <= 0)
    {
        throw mwrs::ProtocolError{
            object_id(),
            Error::invalid_size,
            "Invalid xdg_surface size %dx%d", width, height};
    }
    if (window_role_)
    {
        window_role_.value().set_pending_offset(geom::Displacement{-x, -y});
        window_role_.value().set_pending_width(geom::Width{width});
        window_role_.value().set_pending_height(geom::Height{height});
    }
}

auto mf::XdgSurfaceStable::ack_configure(uint32_t serial) -> void
{
    auto const it = std::find(unacked_configure_serials.begin(), unacked_configure_serials.end(), serial);
    if (it == unacked_configure_serials.end())
    {
        throw mwrs::ProtocolError{
            object_id(),
            Error::invalid_serial,
            "ack_configure with serial %u that was never sent or has already been acked", serial};
    }
    // Acking a serial consumes it and every configure serial sent before it.
    unacked_configure_serials.erase(unacked_configure_serials.begin(), std::next(it));
}

void mf::XdgSurfaceStable::send_configure()
{
    auto const serial = client->next_serial(nullptr);
    unacked_configure_serials.push_back(serial);
    send_configure_event(serial);
}

auto mf::XdgSurfaceStable::window_role() -> mw::Weak<mf::WindowWlSurfaceRole> const&
{
    return window_role_;
}

// XdgPopupStable

mf::XdgPopupStable::XdgPopupStable(
    std::shared_ptr<mwrs::Client> client,
    rust::Box<mwrs::XdgPopupMiddleware> instance,
    uint32_t object_id,
    XdgSurfaceStable* xdg_surface,
    std::optional<WlSurfaceRole*> parent_role,
    XdgPositionerStable& positioner,
    WlSurface* surface)
    : mwrs::XdgPopup{std::move(client), std::move(instance), object_id},
      WindowWlSurfaceRole(
          xdg_surface->xdg_shell.wayland_executor,
          &xdg_surface->xdg_shell.seat,
          mwrs::XdgPopup::client,
          surface,
          xdg_surface->xdg_shell.shell,
          xdg_surface->xdg_shell.output_manager,
          xdg_surface->xdg_shell.surface_registry),
      reactive{positioner.reactive},
      aux_rect{positioner.aux_rect ? positioner.aux_rect.value() : geom::Rectangle{}},
      shell{xdg_surface->xdg_shell.shell},
      xdg_surface{mwrs::make_weak(xdg_surface)}
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

auto mf::XdgPopupStable::grab(mwrs::Weak<mwrs::Seat> const& seat, uint32_t serial) -> void
{
    (void)seat, (void)serial;
    set_type(mir_window_type_menu);
}

auto mf::XdgPopupStable::reposition(mwrs::Weak<mwrs::XdgPositioner> const& positioner_resource, uint32_t token) -> void
{
    auto* positioner = mwrs::XdgPositioner::from<XdgPositionerStable>(positioner_resource);
    positioner->ensure_complete();

    reactive = positioner->reactive;
    aux_rect = positioner->aux_rect ? positioner->aux_rect.value() : geom::Rectangle{};
    reposition_token = token;

    auto const scene_surface_{scene_surface()};
    if (scene_surface_)
    {
        if (auto const session = scene_surface_.value()->session().lock())
        {
            shell->modify_surface(session, scene_surface_.value(), *positioner);
        }
    }
    else
    {
        apply_spec(*positioner);
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
            send_repositioned_event_if_supported(reposition_token.value());
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

auto mf::XdgPopupStable::from(mwrs::Weak<mwrs::XdgPopup> const& resource) -> XdgPopupStable*
{
    auto popup = mwrs::XdgPopup::from<XdgPopupStable>(resource);
    if (!popup)
        throw std::runtime_error("Invalid resource given to XdgPopupStable::from()");
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
    const_cast<XdgPopupStable*>(this)->destroy_and_delete();
}

// XdgToplevelStable

mf::XdgToplevelStable::XdgToplevelStable(
    std::shared_ptr<mwrs::Client> client,
    rust::Box<mwrs::XdgToplevelMiddleware> instance,
    uint32_t object_id,
    XdgSurfaceStable* xdg_surface,
    WlSurface* surface)
    : mwrs::XdgToplevel{std::move(client), std::move(instance), object_id},
      WindowWlSurfaceRole(
          xdg_surface->xdg_shell.wayland_executor,
          &xdg_surface->xdg_shell.seat,
          mwrs::XdgToplevel::client,
          surface,
          xdg_surface->xdg_shell.shell,
          xdg_surface->xdg_shell.output_manager,
          xdg_surface->xdg_shell.surface_registry),
      xdg_surface{mwrs::make_weak(xdg_surface)}
{
    std::vector<uint8_t> capabilities;
    append_u32(capabilities, WmCapabilities::maximize);
    append_u32(capabilities, WmCapabilities::minimize);
    append_u32(capabilities, WmCapabilities::fullscreen);
    send_wm_capabilities_event_if_supported(capabilities);

    send_configure_event(0, 0, {});
    xdg_surface->send_configure();
}

auto mf::XdgToplevelStable::set_parent(std::optional<mwrs::Weak<mwrs::XdgToplevel>> const& parent) -> void
{
    if (parent)
    {
        auto parent_toplevel = XdgToplevelStable::from(parent.value());

        if (parent_toplevel == this)
        {
            throw mwrs::ProtocolError{
                object_id(),
                Error::invalid_parent,
                "A toplevel cannot be its own parent"};
        }

        // Check that the parent is not a descendant of this toplevel
        if (auto const this_surface = scene_surface().value_or(nullptr))
        {
            for (auto ancestor = parent_toplevel->scene_surface().value_or(nullptr); ancestor; ancestor = ancestor->parent())
            {
                if (ancestor == this_surface)
                {
                    throw mwrs::ProtocolError{
                        object_id(),
                        Error::invalid_parent,
                        "Parent toplevel must not be a descendant of the child toplevel"};
                }
            }
        }

        WindowWlSurfaceRole::set_parent(parent_toplevel->scene_surface());
    }
    else
    {
        WindowWlSurfaceRole::set_parent({});
    }
}

auto mf::XdgToplevelStable::set_title(rust::String title) -> void
{
    WindowWlSurfaceRole::set_title(std::string{title});
}

auto mf::XdgToplevelStable::set_app_id(rust::String app_id) -> void
{
    WindowWlSurfaceRole::set_application_id(std::string{app_id});
}

auto mf::XdgToplevelStable::show_window_menu(
    mwrs::Weak<mwrs::Seat> const& seat,
    uint32_t serial,
    int32_t x,
    int32_t y) -> void
{
    (void)seat, (void)serial, (void)x, (void)y;
    // TODO
}

auto mf::XdgToplevelStable::r_move(mwrs::Weak<mwrs::Seat> const& /*seat*/, uint32_t serial) -> void
{
    initiate_interactive_move(serial);
}

auto mf::XdgToplevelStable::resize(mwrs::Weak<mwrs::Seat> const& /*seat*/, uint32_t serial, uint32_t edges) -> void
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
        throw mwrs::ProtocolError{
            object_id(),
            Error::invalid_resize_edge,
            "Invalid resize edge %d", edges};
    }

    initiate_interactive_resize(edge, serial);
}

auto mf::XdgToplevelStable::set_max_size(int32_t width, int32_t height) -> void
{
    if (width < 0 || height < 0)
    {
        throw mwrs::ProtocolError{
            object_id(),
            Error::invalid_size,
            "Invalid maximum size %dx%d", width, height};
    }
    WindowWlSurfaceRole::set_max_size(width, height);
}

auto mf::XdgToplevelStable::set_min_size(int32_t width, int32_t height) -> void
{
    if (width < 0 || height < 0)
    {
        throw mwrs::ProtocolError{
            object_id(),
            Error::invalid_size,
            "Invalid minimum size %dx%d", width, height};
    }
    WindowWlSurfaceRole::set_min_size(width, height);
}

void mf::XdgToplevelStable::handle_commit()
{
    auto const min_size = pending_min_size();
    auto const max_size = pending_max_size();

    // A zero max dimension means "no limit" (stored as the maximum possible size) and a zero min means
    // "no minimum", so this comparison only triggers when the client has set genuinely conflicting constraints.
    if (max_size.width < min_size.width || max_size.height < min_size.height)
    {
        throw mwrs::ProtocolError{
            object_id(),
            Error::invalid_size,
            "Maximum size %dx%d is smaller than minimum size %dx%d",
            max_size.width.as_int(), max_size.height.as_int(),
            min_size.width.as_int(), min_size.height.as_int()};
    }
}

auto mf::XdgToplevelStable::set_maximized() -> void
{
    // We must process this request immediately (i.e. don't defer until commit())
    add_state_now(mir_window_state_maximized);
}

auto mf::XdgToplevelStable::unset_maximized() -> void
{
    // We must process this request immediately (i.e. don't defer until commit())
    remove_state_now(mir_window_state_maximized);
}

auto mf::XdgToplevelStable::set_fullscreen(std::optional<mwrs::Weak<mwrs::Output>> const& output) -> void
{
    WindowWlSurfaceRole::set_fullscreen(output);
}

auto mf::XdgToplevelStable::unset_fullscreen() -> void
{
    // We must process this request immediately (i.e. don't defer until commit())
    // TODO: should we instead restore the previous state?
    remove_state_now(mir_window_state_fullscreen);
}

auto mf::XdgToplevelStable::set_minimized() -> void
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
        append_u32(states, State::activated);
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
            append_u32(states, State::maximized);
        }

        if (state.has(mir_window_state_fullscreen))
        {
            append_u32(states, State::fullscreen);
        }

        auto tiled_edges = s->tiled_edges();
        if (tiled_edges & mir_tiled_edge_north)
        {
            append_u32(states, State::tiled_top);
        }
        if (tiled_edges & mir_tiled_edge_east)
        {
            append_u32(states, State::tiled_right);
        }
        if (tiled_edges & mir_tiled_edge_south)
        {
            append_u32(states, State::tiled_bottom);
        }
        if (tiled_edges & mir_tiled_edge_west)
        {
            append_u32(states, State::tiled_left);
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

auto mf::XdgToplevelStable::from(mwrs::Weak<mwrs::XdgToplevel> const& surface) -> XdgToplevelStable*
{
    return mwrs::XdgToplevel::from<XdgToplevelStable>(surface);
}

void mf::XdgToplevelStable::destroy_role() const
{
    const_cast<XdgToplevelStable*>(this)->destroy_and_delete();
}

// XdgPositionerStable

mf::XdgPositionerStable::XdgPositionerStable(
    std::shared_ptr<mwrs::Client> client,
    rust::Box<mwrs::XdgPositionerMiddleware> instance,
    uint32_t object_id)
    : mwrs::XdgPositioner{std::move(client), std::move(instance), object_id}
{
    // specifying gravity is not required by the xdg shell protocol, but is by Mir window managers
    surface_placement_gravity = mir_placement_gravity_center;
    aux_rect_placement_gravity = mir_placement_gravity_center;
    placement_hints = static_cast<MirPlacementHints>(0);
}

void mf::XdgPositionerStable::ensure_complete()
{
    if (!width || !height || !aux_rect)
    {
        throw mwrs::ProtocolError{
            object_id(),
            mwrs::XdgWmBase::Error::invalid_positioner,
            "Incomplete positioner"};
    }
}

auto mf::XdgPositionerStable::set_size(int32_t width, int32_t height) -> void
{
    if (width <= 0 || height <= 0)
    {
        throw mwrs::ProtocolError{
            object_id(),
            mwrs::XdgPositioner::Error::invalid_input,
            "Invalid popup positioner size: %dx%d", width, height};
    }
    this->width = geom::Width{width};
    this->height = geom::Height{height};
}

auto mf::XdgPositionerStable::set_anchor_rect(int32_t x, int32_t y, int32_t width, int32_t height) -> void
{
    if (width < 0 || height < 0)
    {
        throw mwrs::ProtocolError{
            object_id(),
            mwrs::XdgPositioner::Error::invalid_input,
            "Invalid popup anchor rect size: %dx%d", width, height};
    }
    aux_rect = geom::Rectangle{{x, y}, {width, height}};
}

auto mf::XdgPositionerStable::set_anchor(uint32_t anchor) -> void
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
            throw mwrs::ProtocolError{
                object_id(),
                mwrs::XdgPositioner::Error::invalid_input,
                "Invalid anchor value %u", anchor};
    }

    aux_rect_placement_gravity = placement;
}

auto mf::XdgPositionerStable::set_gravity(uint32_t gravity) -> void
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
            throw mwrs::ProtocolError{
                object_id(),
                mwrs::XdgPositioner::Error::invalid_input,
                "Invalid gravity value %d", gravity};
    }

    surface_placement_gravity = placement;
}

auto mf::XdgPositionerStable::set_constraint_adjustment(uint32_t constraint_adjustment) -> void
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

auto mf::XdgPositionerStable::set_offset(int32_t x, int32_t y) -> void
{
    aux_rect_placement_offset_x = x;
    aux_rect_placement_offset_y = y;
}

auto mf::XdgPositionerStable::set_reactive() -> void
{
    reactive = true;
}

auto mf::XdgPositionerStable::set_parent_size(int32_t parent_width, int32_t parent_height) -> void
{
    if (parent_width <= 0 || parent_height <= 0)
    {
        throw mwrs::ProtocolError{
            object_id(),
            Error::invalid_input,
            "Invalid popup positioner parent size: %dx%d", parent_width, parent_height};
    }
    parent_size = geom::Size{parent_width, parent_height};
}

auto mf::XdgPositionerStable::set_parent_configure(uint32_t /*serial*/) -> void
{
    // TODO
    log_warning("xdg_positioner.set_parent_configure not implemented");
}

auto mf::XdgShellStable::get_window(
    mwrs::Weak<mwrs::XdgSurface> const& surface) -> std::shared_ptr<scene::Surface>
{
    auto const xdgsurface = XdgSurfaceStable::from(surface);
    if (xdgsurface)
    {
        if (auto const& role = xdgsurface->window_role())
        {
            if (auto const scene_surface = role.value().scene_surface())
            {
                return scene_surface.value();
            }
        }

        log_debug("No window currently associated with wayland_rs::XdgSurface@%u", xdgsurface->object_id());
    }

    return {};
}
