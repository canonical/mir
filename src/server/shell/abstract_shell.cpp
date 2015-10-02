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
#include "mir/shell/window_manager.h"
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
    std::shared_ptr<ms::SurfaceCoordinator> const& surface_coordinator,
    std::shared_ptr<ms::SessionCoordinator> const& session_coordinator,
    std::shared_ptr<ms::PromptSessionManager> const& prompt_session_manager,
    std::function<std::shared_ptr<shell::WindowManager>(FocusController* focus_controller)> const& wm_builder) :
    input_targeter(input_targeter),
    surface_coordinator(surface_coordinator),
    session_coordinator(session_coordinator),
    prompt_session_manager(prompt_session_manager),
    window_manager(wm_builder(this))
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
    auto const result = session_coordinator->open_session(client_pid, name, sink);
    window_manager->add_session(result);
    return result;
}

void msh::AbstractShell::close_session(
    std::shared_ptr<ms::Session> const& session)
{
    prompt_session_manager->remove_session(session);
    session_coordinator->close_session(session);
    window_manager->remove_session(session);
}

mf::SurfaceId msh::AbstractShell::create_surface(
    std::shared_ptr<ms::Session> const& session,
    ms::SurfaceCreationParameters const& params,
    std::shared_ptr<mf::EventSink> const& sink)
{
    auto const build = [this, sink](std::shared_ptr<ms::Session> const& session, ms::SurfaceCreationParameters const& placed_params)
        {
            return session->create_surface(placed_params, sink);
        };

    return window_manager->add_surface(session, params, build);
}

void msh::AbstractShell::modify_surface(std::shared_ptr<scene::Session> const& session, std::shared_ptr<scene::Surface> const& surface, SurfaceSpecification const& modifications)
{
    window_manager->modify_surface(session, surface, modifications);
}

void msh::AbstractShell::destroy_surface(
    std::shared_ptr<ms::Session> const& session,
    mf::SurfaceId surface)
{
    window_manager->remove_surface(session, session->surface(surface));
    session->destroy_surface(surface);
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
    std::shared_ptr<ms::Session> const& session,
    std::shared_ptr<ms::Surface> const& surface,
    MirSurfaceAttrib attrib,
    int value)
{
    return window_manager->set_surface_attribute(session, surface, attrib, value);
}

int msh::AbstractShell::get_surface_attribute(
    std::shared_ptr<ms::Surface> const& surface,
    MirSurfaceAttrib attrib)
{
    return surface->query(attrib);
}


void msh::AbstractShell::focus_next_session()
{
    std::unique_lock<std::mutex> lock(focus_mutex);
    auto session = focus_session.lock();

    session = session_coordinator->successor_of(session);

    std::shared_ptr<ms::Surface> surface;

    if (session)
        surface = session->default_surface();

    set_focus_to_locked(lock, session, surface);
}

std::shared_ptr<ms::Session> msh::AbstractShell::focused_session() const
{
    std::unique_lock<std::mutex> lg(focus_mutex);
    return focus_session.lock();
}

std::shared_ptr<ms::Surface> msh::AbstractShell::focused_surface() const
{
    std::unique_lock<std::mutex> lock(focus_mutex);
    return focus_surface.lock();
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
    std::shared_ptr<ms::Session> const& session,
    std::shared_ptr<ms::Surface> const& surface)
{
    auto const current_focus = focus_surface.lock();

    if (surface != current_focus)
    {
        focus_surface = surface;

        if (current_focus)
            current_focus->configure(mir_surface_attrib_focus, mir_surface_unfocused);

        if (surface)
        {
            // Ensure the surface has really taken the focus before notifying it that it is focused
            input_targeter->set_focus(surface);
            surface->configure(mir_surface_attrib_focus, mir_surface_focused);
        }
        else
        {
            input_targeter->clear_focus();
        }
    }

    auto const current_session = focus_session.lock();

    if (session != current_session)
    {
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
}

void msh::AbstractShell::add_display(geometry::Rectangle const& area)
{
    window_manager->add_display(area);
}

void msh::AbstractShell::remove_display(geometry::Rectangle const& area)
{
    window_manager->remove_display(area);
}

bool msh::AbstractShell::handle(MirEvent const& event)
{
    if (mir_event_get_type(&event) != mir_event_type_input)
        return false;

    auto const input_event = mir_event_get_input_event(&event);

    switch (mir_input_event_get_type(input_event))
    {
    case mir_input_event_type_key:
        return window_manager->handle_keyboard_event(mir_input_event_get_keyboard_event(input_event));

    case mir_input_event_type_touch:
        return window_manager->handle_touch_event(mir_input_event_get_touch_event(input_event));

    case mir_input_event_type_pointer:
        return window_manager->handle_pointer_event(mir_input_event_get_pointer_event(input_event));
    }

    return false;
}

auto msh::AbstractShell::surface_at(geometry::Point cursor) const
-> std::shared_ptr<scene::Surface>
{
    return surface_coordinator->surface_at(cursor);
}

void msh::AbstractShell::raise(SurfaceSet const& surfaces)
{
    surface_coordinator->raise(surfaces);
}

