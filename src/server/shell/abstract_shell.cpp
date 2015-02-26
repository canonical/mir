/*
 * Copyright © 2015 Canonical Ltd.
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
 * Authored By: Alan Griffiths <alan@octopull.co.uk>
 */

#include "mir/shell/abstract_shell.h"
#include "mir/shell/input_targeter.h"
#include "mir/scene/prompt_session.h"
#include "mir/scene/prompt_session_manager.h"
#include "mir/scene/session_coordinator.h"
#include "mir/scene/session.h"
#include "mir/scene/surface.h"
#include "mir/scene/surface_coordinator.h"

namespace mf = mir::frontend;
namespace ms = mir::scene;
namespace msh = mir::shell;

msh::AbstractShell::AbstractShell(
    std::shared_ptr<InputTargeter> const& input_targeter,
    std::shared_ptr<scene::SurfaceCoordinator> const& surface_coordinator,
    std::shared_ptr<scene::SessionCoordinator> const& session_coordinator,
    std::shared_ptr<scene::PromptSessionManager> const& prompt_session_manager) :
    input_targeter(input_targeter),
    surface_coordinator(surface_coordinator),
    session_coordinator(session_coordinator),
    prompt_session_manager(prompt_session_manager)
{
}

msh::AbstractShell::~AbstractShell() noexcept
{
}

std::shared_ptr<ms::Session> msh::AbstractShell::open_session(
    pid_t client_pid,
    std::string const& name,
    std::shared_ptr<mf::EventSink> const& sink)
{
    return session_coordinator->open_session(client_pid, name, sink);
}

void msh::AbstractShell::close_session(
    std::shared_ptr<ms::Session> const& session)
{
    prompt_session_manager->remove_session(session);
    session_coordinator->close_session(session);
}

mf::SurfaceId msh::AbstractShell::create_surface(
    std::shared_ptr<ms::Session> const& session,
    ms::SurfaceCreationParameters const& params)
{
    return session->create_surface(params);
}

void msh::AbstractShell::destroy_surface(
    std::shared_ptr<ms::Session> const& session,
    mf::SurfaceId surface)
{
    session->destroy_surface(surface);
}

void msh::AbstractShell::handle_surface_created(
    std::shared_ptr<ms::Session> const& /*session*/)
{
}

std::shared_ptr<ms::PromptSession> msh::AbstractShell::start_prompt_session_for(
    std::shared_ptr<ms::Session> const& session,
    scene::PromptSessionCreationParameters const& params)
{
    return prompt_session_manager->start_prompt_session_for(session, params);
}

void msh::AbstractShell::add_prompt_provider_for(
    std::shared_ptr<ms::PromptSession> const& prompt_session,
    std::shared_ptr<ms::Session> const& session)
{
    prompt_session_manager->add_prompt_provider(prompt_session, session);
}

void msh::AbstractShell::stop_prompt_session(
    std::shared_ptr<ms::PromptSession> const& prompt_session)
{
    prompt_session_manager->stop_prompt_session(prompt_session);
}

int msh::AbstractShell::set_surface_attribute(
    std::shared_ptr<ms::Session> const& /*session*/,
    std::shared_ptr<ms::Surface> const& surface,
    MirSurfaceAttrib attrib,
    int value)
{
    return surface->configure(attrib, value);
}

int msh::AbstractShell::get_surface_attribute(
    std::shared_ptr<ms::Surface> const& surface,
    MirSurfaceAttrib attrib)
{
    return surface->query(attrib);
}


void msh::AbstractShell::focus_next()
{
    std::unique_lock<std::mutex> lock(focus_mutex);
    auto focus = focus_session.lock();

    focus = session_coordinator->successor_of(focus);

    set_focus_to_locked(lock, focus);
}

std::weak_ptr<ms::Session> msh::AbstractShell::focussed_application() const
{
    std::unique_lock<std::mutex> lg(focus_mutex);
    return focus_session;
}

std::shared_ptr<ms::Surface> msh::AbstractShell::focused_surface() const
{
    std::unique_lock<std::mutex> lock(focus_mutex);
    return focus_surface.lock();
}

void msh::AbstractShell::set_focus_to(
    std::shared_ptr<scene::Session> const& focus)
{
    std::unique_lock<std::mutex> lg(focus_mutex);
    set_focus_to_locked(lg, focus);
}

void msh::AbstractShell::set_focus_to(
    std::shared_ptr<ms::Session> const& focus_session,
    std::shared_ptr<ms::Surface> const& focus_surface)
{
    std::unique_lock<std::mutex> lock(focus_mutex);

    set_focus_to_locked(lock, focus_session, focus_surface);
}

void msh::AbstractShell::set_focus_to_locked(
    std::unique_lock<std::mutex> const& /*lock*/,
    std::shared_ptr<ms::Surface> const& surface)
{
    setting_focus_to(surface);

    if (surface)
    {
        // Ensure the surface has really taken the focus before notifying it that it is focused
        surface->take_input_focus(input_targeter);
        if (auto current_focus = focus_surface.lock())
            current_focus->configure(mir_surface_attrib_focus, mir_surface_unfocused);

        surface->configure(mir_surface_attrib_focus, mir_surface_focused);
        focus_surface = surface;
    }
    else
    {
        input_targeter->focus_cleared();
    }
}

void msh::AbstractShell::set_focus_to_locked(
    std::unique_lock<std::mutex> const& lock,
    std::shared_ptr<ms::Session> const& session)
{
    std::shared_ptr<ms::Surface> surface;

    if (session)
        surface = session->default_surface();

    set_focus_to_locked(lock, session, surface);
}

void msh::AbstractShell::set_focus_to_locked(
    std::unique_lock<std::mutex> const& lock,
    std::shared_ptr<ms::Session> const& session,
    std::shared_ptr<ms::Surface> const& surface)
{
    setting_focus_to(session);

    auto old_focus = focus_session.lock();

    set_focus_to_locked(lock, surface);

    focus_session = session;

    if (session)
    {
        session_coordinator->set_focus_to(session);
    }
    else
    {
        session_coordinator->unset_focus();
    }
}

void msh::AbstractShell::setting_focus_to(std::shared_ptr<ms::Surface> const& /*surface*/)
{
}

void msh::AbstractShell::setting_focus_to(std::shared_ptr<ms::Session> const& /*session*/)
{
}

void msh::AbstractShell::add_display(geometry::Rectangle const& /*area*/)
{
}

void msh::AbstractShell::remove_display(geometry::Rectangle const& /*area*/)
{
}

bool msh::AbstractShell::handle(MirEvent const& /*event*/)
{
    return false;
}
