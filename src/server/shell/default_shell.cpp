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

#include "default_shell.h"
#include "mir/shell/input_targeter.h"
#include "mir/scene/placement_strategy.h"
#include "mir/scene/prompt_session.h"
#include "mir/scene/prompt_session_manager.h"
#include "mir/scene/session_coordinator.h"
#include "mir/scene/session.h"
#include "mir/scene/surface.h"
#include "mir/scene/surface_coordinator.h"
#include "mir/scene/surface_creation_parameters.h"

#include <boost/throw_exception.hpp>

#include <stdexcept>

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

msh::DefaultShell::DefaultShell(
    std::shared_ptr<InputTargeter> const& input_targeter,
    std::shared_ptr<scene::SurfaceCoordinator> const& surface_coordinator,
    std::shared_ptr<scene::SessionCoordinator> const& session_coordinator,
    std::shared_ptr<scene::PromptSessionManager> const& prompt_session_manager,
    std::shared_ptr<ms::PlacementStrategy> const& placement_strategy) :
    AbstractShell(input_targeter, surface_coordinator, session_coordinator, prompt_session_manager),
    placement_strategy(placement_strategy)
{
}

std::shared_ptr<ms::Session> msh::DefaultShell::open_session(
    pid_t client_pid,
    std::string const& name,
    std::shared_ptr<mf::EventSink> const& sink)
{
    auto const new_session = session_coordinator->open_session(client_pid, name, sink);
    set_focus_to(new_session);
    return new_session;
}

void msh::DefaultShell::close_session(
    std::shared_ptr<ms::Session> const& session)
{
    prompt_session_manager->remove_session(session);
    session_coordinator->close_session(session);
    set_focus_to(session_coordinator->successor_of(std::shared_ptr<ms::Session>()));
}

void msh::DefaultShell::focus_next()
{
    std::unique_lock<std::mutex> lock(focus_application_mutex);
    auto focus = focus_application.lock();

    focus = session_coordinator->successor_of(focus);

    set_focus_to_locked(lock, focus);
}

std::weak_ptr<ms::Session> msh::DefaultShell::focussed_application() const
{
    std::unique_lock<std::mutex> lg(focus_application_mutex);
    return focus_application;
}

void msh::DefaultShell::set_focus_to(
    std::shared_ptr<scene::Session> const& focus)
{
    std::unique_lock<std::mutex> lg(focus_application_mutex);
    set_focus_to_locked(lg, focus);
}

void msh::DefaultShell::handle_surface_created(
    std::shared_ptr<ms::Session> const& session)
{
    set_focus_to(session);
}

std::shared_ptr<ms::PromptSession> msh::DefaultShell::start_prompt_session_for(
    std::shared_ptr<ms::Session> const& session,
    scene::PromptSessionCreationParameters const& params)
{
    return prompt_session_manager->start_prompt_session_for(session, params);
}

void msh::DefaultShell::add_prompt_provider_for(
    std::shared_ptr<ms::PromptSession> const& prompt_session,
    std::shared_ptr<ms::Session> const& session)
{
    prompt_session_manager->add_prompt_provider(prompt_session, session);
}

void msh::DefaultShell::stop_prompt_session(std::shared_ptr<ms::PromptSession> const& prompt_session)
{
    prompt_session_manager->stop_prompt_session(prompt_session);
}

mf::SurfaceId msh::DefaultShell::create_surface(std::shared_ptr<ms::Session> const& session, ms::SurfaceCreationParameters const& params)
{
    auto placed_params = placement_strategy->place(*session, params);
    auto const result = session->create_surface(placed_params);
    return result;
}

void msh::DefaultShell::destroy_surface(std::shared_ptr<ms::Session> const& session, mf::SurfaceId surface)
{
    session->destroy_surface(surface);
}

int msh::DefaultShell::set_surface_attribute(
    std::shared_ptr<ms::Session> const& session,
    mf::SurfaceId surface_id,
    MirSurfaceAttrib attrib,
    int value)
{
    auto const surface = session->surface(surface_id);

    // TODO scene::SurfaceConfigurator is really a DefaultShell strategy
    // TODO it should be invoked from here around any changes to the surface

    return surface->configure(attrib, value);
}

int msh::DefaultShell::get_surface_attribute(
    std::shared_ptr<ms::Session> const& session,
    mf::SurfaceId surface_id,
    MirSurfaceAttrib attrib)
{
    auto const surface = session->surface(surface_id);

    if (!surface)
        BOOST_THROW_EXCEPTION(std::logic_error("invalid surface id"));

    return surface->query(attrib);
}


inline void msh::DefaultShell::set_focus_to_locked(std::unique_lock<std::mutex> const&, std::shared_ptr<ms::Session> const& session)
{
    auto old_focus = focus_application.lock();

    std::shared_ptr<ms::Surface> surface;

    if (session)
        surface = session->default_surface();

    if (surface)
    {
        std::lock_guard<std::mutex> lg(focus_surface_mutex);

        // Ensure the surface has really taken the focus before notifying it that it is focused
        surface_coordinator->raise(surface);
        surface->take_input_focus(input_targeter);

        auto current_focus = focus_surface.lock();
        if (current_focus)
            current_focus->configure(mir_surface_attrib_focus, mir_surface_unfocused);
        surface->configure(mir_surface_attrib_focus, mir_surface_focused);
        focus_surface = surface;
    }
    else
    {
        input_targeter->focus_cleared();
    }

    focus_application = session;

    if (session)
    {
        session_coordinator->set_focus_to(session);
    }
    else
    {
        session_coordinator->unset_focus();
    }
}
