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

#include "frontend_shell.h"

#include "default_shell.h"

namespace mf = mir::frontend;
namespace ms = mir::scene;
namespace msh = mir::shell::detail;

std::shared_ptr<mf::Session> msh::FrontendShell::open_session(
    pid_t client_pid,
    std::string const& name,
    std::shared_ptr<mf::EventSink> const& sink)
{
    return wrapped->open_session(client_pid, name, sink);
}

void msh::FrontendShell::close_session(std::shared_ptr<mf::Session> const& session)
{
    wrapped->close_session(session);
}

void msh::FrontendShell::handle_surface_created(std::shared_ptr<mf::Session> const& session)
{
    wrapped->handle_surface_created(session);
}

std::shared_ptr<mf::PromptSession> msh::FrontendShell::start_prompt_session_for(
    std::shared_ptr<mf::Session> const& session,
    ms::PromptSessionCreationParameters const& params)
{
    return wrapped->start_prompt_session_for(session, params);
}

void msh::FrontendShell::add_prompt_provider_for(
    std::shared_ptr<mf::PromptSession> const& prompt_session,
    std::shared_ptr<mf::Session> const& session)
{
    wrapped->add_prompt_provider_for(prompt_session, session);
}

void msh::FrontendShell::stop_prompt_session(std::shared_ptr<mf::PromptSession> const& prompt_session)
{
    wrapped->stop_prompt_session(prompt_session);
}

mf::SurfaceId msh::FrontendShell::create_surface(std::shared_ptr<mf::Session> const& session, ms::SurfaceCreationParameters const& params)
{
    return wrapped->create_surface(session, params);
}

void msh::FrontendShell::destroy_surface(std::shared_ptr<mf::Session> const& session, mf::SurfaceId surface)
{
    wrapped->destroy_surface(session, surface);
}

int msh::FrontendShell::set_surface_attribute(
    std::shared_ptr<mf::Session> const& session,
    mf::SurfaceId surface_id,
    MirSurfaceAttrib attrib,
    int value)
{
    return wrapped->set_surface_attribute(session, surface_id, attrib, value);
}

int msh::FrontendShell::get_surface_attribute(
    std::shared_ptr<mf::Session> const& session,
    mf::SurfaceId surface_id,
    MirSurfaceAttrib attrib)
{
    return wrapped->get_surface_attribute(session, surface_id, attrib);
}
