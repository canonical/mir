/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
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

#include "mir/shell/shell_wrapper.h"
#include "mir/geometry/point.h"

namespace mf = mir::frontend;
namespace ms = mir::scene;
namespace msh = mir::shell;

msh::ShellWrapper::ShellWrapper(std::shared_ptr<Shell> const& wrapped) :
    wrapped(wrapped)
{
}

auto msh::ShellWrapper::open_session(
    pid_t client_pid,
    Fd socket_fd,
    std::string const& name,
    std::shared_ptr<mf::EventSink> const& sink) -> std::shared_ptr<ms::Session>
{
    return wrapped->open_session(client_pid, socket_fd, name, sink);
}

void msh::ShellWrapper::close_session(std::shared_ptr<ms::Session> const& session)
{
    wrapped->close_session(session);
}

void msh::ShellWrapper::focus_next_session()
{
    wrapped->focus_next_session();
}

void msh::ShellWrapper::focus_prev_session()
{
    wrapped->focus_prev_session();
}

std::shared_ptr<ms::Session> msh::ShellWrapper::focused_session() const
{
    return wrapped->focused_session();
}

void msh::ShellWrapper::set_popup_grab_tree(std::shared_ptr<scene::Surface> const& surface)
{
    wrapped->set_popup_grab_tree(surface);
}

void msh::ShellWrapper::set_focus_to(
    std::shared_ptr<scene::Session> const& focus_session,
    std::shared_ptr<scene::Surface> const& focus_surface)

{
    wrapped->set_focus_to(focus_session, focus_surface);
}

std::shared_ptr<ms::PromptSession> msh::ShellWrapper::start_prompt_session_for(
    std::shared_ptr<ms::Session> const& session,
    scene::PromptSessionCreationParameters const& params)
{
    return wrapped->start_prompt_session_for(session, params);
}

void msh::ShellWrapper::add_prompt_provider_for(
    std::shared_ptr<ms::PromptSession> const& prompt_session,
    std::shared_ptr<ms::Session> const& session)
{
    wrapped->add_prompt_provider_for(prompt_session, session);
}

void msh::ShellWrapper::stop_prompt_session(std::shared_ptr<ms::PromptSession> const& prompt_session)
{
    wrapped->stop_prompt_session(prompt_session);
}

auto msh::ShellWrapper::create_surface(
    std::shared_ptr<ms::Session> const& session,
    wayland::Weak<frontend::WlSurface> const& wayland_surface,
    SurfaceSpecification const& params,
    std::shared_ptr<ms::SurfaceObserver> const& observer,
    Executor* observer_executor) -> std::shared_ptr<ms::Surface>
{
    return wrapped->create_surface(session, wayland_surface, params, observer, observer_executor);
}

void msh::ShellWrapper::surface_ready(std::shared_ptr<ms::Surface> const& surface)
{
    wrapped->surface_ready(surface);
}

void msh::ShellWrapper::modify_surface(
    std::shared_ptr<scene::Session> const& session,
    std::shared_ptr<scene::Surface> const& surface,
    SurfaceSpecification const& modifications)
{
    wrapped->modify_surface(session, surface, modifications);
}

void msh::ShellWrapper::destroy_surface(
    std::shared_ptr<ms::Session> const& session,
    std::shared_ptr<ms::Surface> const& surface)
{
    wrapped->destroy_surface(session, surface);
}

void msh::ShellWrapper::raise_surface(
    std::shared_ptr<ms::Session> const& session,
    std::shared_ptr<ms::Surface> const& surface,
    uint64_t timestamp)
{
    wrapped->raise_surface(session, surface, timestamp);
}

void msh::ShellWrapper::request_move(
    std::shared_ptr<ms::Session> const& session,
    std::shared_ptr<ms::Surface> const& surface,
    MirInputEvent const* event)
{
    wrapped->request_move(session, surface, event);
}

void msh::ShellWrapper::request_resize(
    std::shared_ptr<scene::Session> const& session,
    std::shared_ptr<scene::Surface> const& surface,
    MirInputEvent const* event,
    MirResizeEdge edge)
{
    wrapped->request_resize(session, surface, event, edge);
}

void msh::ShellWrapper::add_display(geometry::Rectangle const& area)
{
    wrapped->add_display(area);
}

void msh::ShellWrapper::remove_display(geometry::Rectangle const& area)
{
    wrapped->remove_display(area);
}

bool msh::ShellWrapper::handle(MirEvent const& event)
{
    return wrapped->handle(event);
}

auto msh::ShellWrapper::focused_surface() const -> std::shared_ptr<scene::Surface>
{
    return wrapped->focused_surface();
}

auto msh::ShellWrapper::surface_at(geometry::Point cursor) const -> std::shared_ptr<scene::Surface>
{
    return wrapped->surface_at(cursor);
}

void msh::ShellWrapper::raise(SurfaceSet const& surfaces)
{
    return wrapped->raise(surfaces);
}

void msh::ShellWrapper::swap_z_order(SurfaceSet const& first, SurfaceSet const& second)
{
    return wrapped->swap_z_order(first, second);
}

void msh::ShellWrapper::send_to_back(const mir::shell::SurfaceSet &surfaces)
{
    return wrapped->send_to_back(surfaces);
}

auto msh::ShellWrapper::is_above(std::weak_ptr<scene::Surface> const& a, std::weak_ptr<scene::Surface> const& b) const
    -> bool
{
    return wrapped->is_above(a, b);
}
