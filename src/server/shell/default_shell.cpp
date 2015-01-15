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
#include "mir/scene/prompt_session.h"
#include "mir/scene/session_coordinator.h"
#include "mir/scene/session.h"
#include "mir/scene/surface.h"

#include <boost/throw_exception.hpp>

#include <stdexcept>

namespace mf = mir::frontend;
namespace ms = mir::scene;
namespace msh = mir::shell;

msh::DefaultShell::DefaultShell(
    std::shared_ptr<scene::SessionCoordinator> const& wrapped) :
    wrapped(wrapped)
{
}


std::shared_ptr<mf::Session> msh::DefaultShell::open_session(
    pid_t client_pid,
    std::string const& name,
    std::shared_ptr<mf::EventSink> const& sink)
{
    return wrapped->open_session(client_pid, name, sink);
}

void msh::DefaultShell::close_session(
    std::shared_ptr<mf::Session> const& session)
{
    wrapped->close_session(std::dynamic_pointer_cast<ms::Session>(session));
}

void msh::DefaultShell::focus_next()
{
    wrapped->focus_next();
}

std::weak_ptr<ms::Session> msh::DefaultShell::focussed_application() const
{
    return wrapped->focussed_application();
}

void msh::DefaultShell::set_focus_to(
    std::shared_ptr<scene::Session> const& focus)
{
    wrapped->set_focus_to(focus);
}

void msh::DefaultShell::handle_surface_created(
    std::shared_ptr<mf::Session> const& session)
{
    wrapped->handle_surface_created(std::dynamic_pointer_cast<ms::Session>(session));
}

std::shared_ptr<mf::PromptSession> msh::DefaultShell::start_prompt_session_for(
    std::shared_ptr<mf::Session> const& session,
    scene::PromptSessionCreationParameters const& params)
{
    auto const scene_session = std::dynamic_pointer_cast<ms::Session>(session);

    return wrapped->start_prompt_session_for(scene_session, params);
}

void msh::DefaultShell::add_prompt_provider_for(
    std::shared_ptr<mf::PromptSession> const& prompt_session,
    std::shared_ptr<mf::Session> const& session)
{
    auto const scene_prompt_session = std::dynamic_pointer_cast<ms::PromptSession>(prompt_session);
    auto const scene_session = std::dynamic_pointer_cast<ms::Session>(session);
    wrapped->add_prompt_provider_for(scene_prompt_session, scene_session);
}

void msh::DefaultShell::stop_prompt_session(std::shared_ptr<mf::PromptSession> const& prompt_session)
{
    auto const scene_prompt_session = std::dynamic_pointer_cast<ms::PromptSession>(prompt_session);
    wrapped->stop_prompt_session(scene_prompt_session);
}

mf::SurfaceId msh::DefaultShell::create_surface(std::shared_ptr<mf::Session> const& session, ms::SurfaceCreationParameters const& params)
{
    auto const scene_session = std::dynamic_pointer_cast<ms::Session>(session);
    return wrapped->create_surface(scene_session, params);
}

void msh::DefaultShell::destroy_surface(std::shared_ptr<mf::Session> const& session, mf::SurfaceId surface)
{
    auto const scene_session = std::dynamic_pointer_cast<ms::Session>(session);
    wrapped->destroy_surface(scene_session, surface);
}

int msh::DefaultShell::set_surface_attribute(
    std::shared_ptr<mf::Session> const& session,
    mf::SurfaceId surface_id,
    MirSurfaceAttrib attrib,
    int value)
{
    // TODO this downcasting is clunky, but is temporary wiring of the new interaction route to the old implementation
    if (!session)
        BOOST_THROW_EXCEPTION(std::logic_error("invalid session"));

    auto const surface = std::dynamic_pointer_cast<ms::Surface>(session->get_surface(surface_id));

    if (!surface)
        BOOST_THROW_EXCEPTION(std::logic_error("invalid surface id"));

    return surface->configure(attrib, value);
}

int msh::DefaultShell::get_surface_attribute(
    std::shared_ptr<mf::Session> const& session,
    mf::SurfaceId surface_id,
    MirSurfaceAttrib attrib)
{
    // TODO this downcasting is clunky, but is temporary wiring of the new interaction route to the old implementation
    if (!session)
        BOOST_THROW_EXCEPTION(std::logic_error("invalid session"));

    auto const surface = std::dynamic_pointer_cast<ms::Surface>(session->get_surface(surface_id));

    if (!surface)
        BOOST_THROW_EXCEPTION(std::logic_error("invalid surface id"));

    return surface->query(attrib);
}
