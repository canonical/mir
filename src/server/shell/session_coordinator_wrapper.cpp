/*
 * Copyright © 2014 Canonical Ltd.
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

#include "mir/shell/session_coordinator_wrapper.h"

namespace mf = mir::frontend;
namespace ms = mir::scene;
namespace msh = mir::shell;

msh::SessionCoordinatorWrapper::SessionCoordinatorWrapper(
    std::shared_ptr<scene::SessionCoordinator> const& wrapped) :
    wrapped(wrapped)
{
}


std::shared_ptr<mf::Session> msh::SessionCoordinatorWrapper::open_session(
    pid_t client_pid,
    std::string const& name,
    std::shared_ptr<mf::EventSink> const& sink)
{
    return wrapped->open_session(client_pid, name, sink);
}

void msh::SessionCoordinatorWrapper::close_session(
    std::shared_ptr<mf::Session> const& session)
{
    wrapped->close_session(session);
}

void msh::SessionCoordinatorWrapper::focus_next()
{
    wrapped->focus_next();
}

std::weak_ptr<ms::Session> msh::SessionCoordinatorWrapper::focussed_application() const
{
    return wrapped->focussed_application();
}

void msh::SessionCoordinatorWrapper::set_focus_to(
    std::shared_ptr<scene::Session> const& focus)
{
    wrapped->set_focus_to(focus);
}

void msh::SessionCoordinatorWrapper::handle_surface_created(
    std::shared_ptr<mf::Session> const& session)
{
    wrapped->handle_surface_created(session);
}

std::shared_ptr<mf::PromptSession> msh::SessionCoordinatorWrapper::start_prompt_session_for(
    std::shared_ptr<mf::Session> const& session,
    scene::PromptSessionCreationParameters const& params)
{
    return wrapped->start_prompt_session_for(session, params);
}

void msh::SessionCoordinatorWrapper::add_prompt_provider_for(
    std::shared_ptr<mf::PromptSession> const& prompt_session,
    std::shared_ptr<mf::Session> const& session)
{
    wrapped->add_prompt_provider_for(prompt_session, session);
}

void msh::SessionCoordinatorWrapper::stop_prompt_session(std::shared_ptr<mf::PromptSession> const& prompt_session)
{
    wrapped->stop_prompt_session(prompt_session);
}

mf::SurfaceId msh::SessionCoordinatorWrapper::create_surface(std::shared_ptr<mf::Session> const& session, ms::SurfaceCreationParameters const& params)
{
    return wrapped->create_surface(session, params);
}

void msh::SessionCoordinatorWrapper::destroy_surface(std::shared_ptr<mf::Session> const& session, mf::SurfaceId surface)
{
    wrapped->destroy_surface(session, surface);
}

int msh::SessionCoordinatorWrapper::set_surface_attribute(
    std::shared_ptr<mf::Session> const& session,
    mf::SurfaceId surface_id,
    MirSurfaceAttrib attrib,
    int value)
{
    return wrapped->set_surface_attribute(session, surface_id, attrib, value);
}

int msh::SessionCoordinatorWrapper::get_surface_attribute(
    std::shared_ptr<mf::Session> const& session,
    mf::SurfaceId surface_id,
    MirSurfaceAttrib attrib)
{
    return wrapped->get_surface_attribute(session, surface_id, attrib);
}
