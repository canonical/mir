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
#include "mir/shell/shell_report.h"
#include "mir/shell/surface_specification.h"
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
    std::shared_ptr<ShellReport> const& report,
    std::function<std::shared_ptr<shell::WindowManager>(FocusController* focus_controller)> const& wm_builder) :
    input_targeter(input_targeter),
    surface_coordinator(surface_coordinator),
    session_coordinator(session_coordinator),
    prompt_session_manager(prompt_session_manager),
    window_manager(wm_builder(this)),
    report(report)
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
    report->opened_session(*result);
    return result;
}

void msh::AbstractShell::close_session(
    std::shared_ptr<ms::Session> const& session)
{
    report->closing_session(*session);
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

    auto const result = window_manager->add_surface(session, params, build);
    report->created_surface(*session, result);
    return result;
}

void msh::AbstractShell::modify_surface(std::shared_ptr<scene::Session> const& session, std::shared_ptr<scene::Surface> const& surface, SurfaceSpecification const& modifications)
{
    report->update_surface(*session, *surface, modifications);

    auto wm_relevant_mods = modifications;
    if (wm_relevant_mods.streams.is_set())
    {
        session->configure_streams(*surface, wm_relevant_mods.streams.consume());
    }
    if (!wm_relevant_mods.is_empty())
    {
        window_manager->modify_surface(session, surface, wm_relevant_mods);
    }
}

void msh::AbstractShell::destroy_surface(
    std::shared_ptr<ms::Session> const& session,
    mf::SurfaceId surface)
{
    report->destroying_surface(*session, surface);
    // Order matters. The surface needs to be destroyed before calling the WM,
    // so the WM and its tools can make decisions based on up-to-date information.
    auto weak_surface = std::weak_ptr<ms::Surface>{session->surface(surface)};
    session->destroy_surface(surface);
    window_manager->remove_surface(session, weak_surface);
}

std::shared_ptr<ms::PromptSession> msh::AbstractShell::start_prompt_session_for(
    std::shared_ptr<ms::Session> const& session,
    scene::PromptSessionCreationParameters const& params)
{
    auto const result = prompt_session_manager->start_prompt_session_for(session, params);
    report->started_prompt_session(*result, *session);
    return result;
}

void msh::AbstractShell::add_prompt_provider_for(
    std::shared_ptr<ms::PromptSession> const& prompt_session,
    std::shared_ptr<ms::Session> const& session)
{
    prompt_session_manager->add_prompt_provider(prompt_session, session);
    report->added_prompt_provider(*prompt_session, *session);
}

void msh::AbstractShell::stop_prompt_session(
    std::shared_ptr<ms::PromptSession> const& prompt_session)
{
    report->stopping_prompt_session(*prompt_session);
    prompt_session_manager->stop_prompt_session(prompt_session);
}

int msh::AbstractShell::set_surface_attribute(
    std::shared_ptr<ms::Session> const& session,
    std::shared_ptr<ms::Surface> const& surface,
    MirSurfaceAttrib attrib,
    int value)
{
    report->update_surface(*session, *surface, attrib, value);
    return window_manager->set_surface_attribute(session, surface, attrib, value);
}

int msh::AbstractShell::get_surface_attribute(
    std::shared_ptr<ms::Surface> const& surface,
    MirSurfaceAttrib attrib)
{
    return surface->query(attrib);
}

void msh::AbstractShell::raise_surface_with_timestamp(
    std::shared_ptr<ms::Session> const& session,
    std::shared_ptr<ms::Surface> const& surface,
    uint64_t timestamp)
{
    window_manager->handle_raise_surface(session, surface, timestamp);
}

void msh::AbstractShell::focus_next_session()
{
    std::unique_lock<std::mutex> lock(focus_mutex);
    auto const focused_session = focus_session.lock();
    auto successor = session_coordinator->successor_of(focused_session);

    while (successor != nullptr &&
           successor != focused_session &&
           successor->default_surface() == nullptr)
    {
        successor = session_coordinator->successor_of(successor);
    }

    auto const surface = successor ? successor->default_surface() : nullptr;

    if (!surface)
        successor = nullptr;

    set_focus_to_locked(lock, successor, surface);
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

    report->input_focus_set_to(session.get(), surface.get());
}

void msh::AbstractShell::add_display(geometry::Rectangle const& area)
{
    report->adding_display(area);
    window_manager->add_display(area);
}

void msh::AbstractShell::remove_display(geometry::Rectangle const& area)
{
    report->removing_display(area);
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
    report->surfaces_raised(surfaces);
}
