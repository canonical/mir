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

#include "shell_with_surface_registry.h"
#include "surface_registry.h"
#include "wl_surface.h"

#include <mir/geometry/point.h>
#include <mir/geometry/rectangle.h>
#include <mir/graphics/display_configuration.h>
#include <mir/scene/session.h>
#include <mir/scene/surface.h>
#include <mir/wayland/weak.h>

namespace mf = mir::frontend;
namespace ms = mir::scene;
namespace msh = mir::shell;
namespace mw = mir::wayland;

mf::ShellWithSurfaceRegistry::ShellWithSurfaceRegistry(
    std::shared_ptr<msh::Shell> const& wrapped_shell,
    std::shared_ptr<SurfaceRegistry> const& surface_registry)
    : wrapped_shell{wrapped_shell},
      registry{surface_registry}
{
}

auto mf::ShellWithSurfaceRegistry::open_session(
    pid_t client_pid,
    Fd socket_fd,
    std::string const& name) -> std::shared_ptr<ms::Session>
{
    return wrapped_shell->open_session(client_pid, socket_fd, name);
}

void mf::ShellWithSurfaceRegistry::close_session(std::shared_ptr<ms::Session> const& session)
{
    wrapped_shell->close_session(session);
}

std::shared_ptr<ms::PromptSession> mf::ShellWithSurfaceRegistry::start_prompt_session_for(
    std::shared_ptr<ms::Session> const& session,
    ms::PromptSessionCreationParameters const& params)
{
    return wrapped_shell->start_prompt_session_for(session, params);
}

void mf::ShellWithSurfaceRegistry::add_prompt_provider_for(
    std::shared_ptr<ms::PromptSession> const& prompt_session,
    std::shared_ptr<ms::Session> const& session)
{
    wrapped_shell->add_prompt_provider_for(prompt_session, session);
}

void mf::ShellWithSurfaceRegistry::stop_prompt_session(std::shared_ptr<ms::PromptSession> const& prompt_session)
{
    wrapped_shell->stop_prompt_session(prompt_session);
}

auto mf::ShellWithSurfaceRegistry::create_surface(
    std::shared_ptr<ms::Session> const& session,
    msh::SurfaceSpecification const& params,
    std::shared_ptr<ms::SurfaceObserver> const& observer,
    Executor* observer_executor) -> std::shared_ptr<ms::Surface>
{
    // Note: The caller must call add_surface_to_registry() with the WlSurface after this
    // to complete the registration. The asymmetry with destroy_surface() (which automatically
    // removes from registry) is intentional: we don't have access to the WlSurface at this point.
    return wrapped_shell->create_surface(session, params, observer, observer_executor);
}

void mf::ShellWithSurfaceRegistry::surface_ready(std::shared_ptr<ms::Surface> const& surface)
{
    wrapped_shell->surface_ready(surface);
}

void mf::ShellWithSurfaceRegistry::modify_surface(
    std::shared_ptr<ms::Session> const& session,
    std::shared_ptr<ms::Surface> const& surface,
    msh::SurfaceSpecification const& modifications)
{
    wrapped_shell->modify_surface(session, surface, modifications);
}

void mf::ShellWithSurfaceRegistry::destroy_surface(
    std::shared_ptr<ms::Session> const& session,
    std::shared_ptr<ms::Surface> const& surface)
{
    // Remove from registry first, then destroy
    registry->remove_surface(surface);
    wrapped_shell->destroy_surface(session, surface);
}

void mf::ShellWithSurfaceRegistry::raise_surface(
    std::shared_ptr<ms::Session> const& session,
    std::shared_ptr<ms::Surface> const& surface,
    uint64_t timestamp)
{
    wrapped_shell->raise_surface(session, surface, timestamp);
}

void mf::ShellWithSurfaceRegistry::request_move(
    std::shared_ptr<ms::Session> const& session,
    std::shared_ptr<ms::Surface> const& surface,
    MirInputEvent const* event)
{
    wrapped_shell->request_move(session, surface, event);
}

void mf::ShellWithSurfaceRegistry::request_resize(
    std::shared_ptr<ms::Session> const& session,
    std::shared_ptr<ms::Surface> const& surface,
    MirInputEvent const* event,
    MirResizeEdge edge)
{
    wrapped_shell->request_resize(session, surface, event, edge);
}

void mf::ShellWithSurfaceRegistry::focus_next_session()
{
    wrapped_shell->focus_next_session();
}

void mf::ShellWithSurfaceRegistry::focus_prev_session()
{
    wrapped_shell->focus_prev_session();
}

auto mf::ShellWithSurfaceRegistry::focused_session() const -> std::shared_ptr<ms::Session>
{
    return wrapped_shell->focused_session();
}

auto mf::ShellWithSurfaceRegistry::focused_surface() const -> std::shared_ptr<ms::Surface>
{
    return wrapped_shell->focused_surface();
}

void mf::ShellWithSurfaceRegistry::set_focus_to(
    std::shared_ptr<ms::Session> const& focus_session,
    std::shared_ptr<ms::Surface> const& focus_surface)
{
    wrapped_shell->set_focus_to(focus_session, focus_surface);
}

void mf::ShellWithSurfaceRegistry::set_popup_grab_tree(std::shared_ptr<ms::Surface> const& surface)
{
    wrapped_shell->set_popup_grab_tree(surface);
}

auto mf::ShellWithSurfaceRegistry::surface_at(geometry::Point cursor) const -> std::shared_ptr<ms::Surface>
{
    return wrapped_shell->surface_at(cursor);
}

void mf::ShellWithSurfaceRegistry::raise(msh::SurfaceSet const& surfaces)
{
    wrapped_shell->raise(surfaces);
}

void mf::ShellWithSurfaceRegistry::swap_z_order(msh::SurfaceSet const& first, msh::SurfaceSet const& second)
{
    wrapped_shell->swap_z_order(first, second);
}

void mf::ShellWithSurfaceRegistry::send_to_back(msh::SurfaceSet const& surfaces)
{
    wrapped_shell->send_to_back(surfaces);
}

auto mf::ShellWithSurfaceRegistry::is_above(
    std::weak_ptr<ms::Surface> const& a,
    std::weak_ptr<ms::Surface> const& b) const -> bool
{
    return wrapped_shell->is_above(a, b);
}

auto mf::ShellWithSurfaceRegistry::handle(MirEvent const& event) -> bool
{
    return wrapped_shell->handle(event);
}

void mf::ShellWithSurfaceRegistry::add_display(geometry::Rectangle const& area)
{
    wrapped_shell->add_display(area);
}

void mf::ShellWithSurfaceRegistry::remove_display(geometry::Rectangle const& area)
{
    wrapped_shell->remove_display(area);
}

void mf::ShellWithSurfaceRegistry::add_surface_to_registry(
    std::shared_ptr<ms::Surface> const& surface,
    mw::Weak<WlSurface> const& wl_surface)
{
    registry->add_surface(surface, wl_surface);
}
