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

#include "mir/shell/shell_wrapper.h"

namespace mf = mir::frontend;
namespace ms = mir::scene;
namespace msh = mir::shell;

msh::ShellWrapper::ShellWrapper(std::shared_ptr<Shell> const& wrapped) :
    wrapped(wrapped)
{
}

std::shared_ptr<ms::Session> msh::ShellWrapper::open_session(
    pid_t client_pid,
    std::string const& name,
    std::shared_ptr<mf::EventSink> const& sink)
{
    return wrapped->open_session(client_pid, name, sink);
}

void msh::ShellWrapper::close_session(std::shared_ptr<ms::Session> const& session)
{
    wrapped->close_session(session);
}

void msh::ShellWrapper::focus_next()
{
    wrapped->focus_next();
}

std::weak_ptr<ms::Session> msh::ShellWrapper::focussed_application() const
{
    return wrapped->focussed_application();
}

void msh::ShellWrapper::set_focus_to(std::shared_ptr<scene::Session> const& focus)
{
    wrapped->set_focus_to(focus);
}

void msh::ShellWrapper::handle_surface_created(std::shared_ptr<ms::Session> const& session)
{
    wrapped->handle_surface_created(session);
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

mf::SurfaceId msh::ShellWrapper::create_surface(std::shared_ptr<ms::Session> const& session, ms::SurfaceCreationParameters const& params)
{
    return wrapped->create_surface(session, params);
}

void msh::ShellWrapper::destroy_surface(std::shared_ptr<ms::Session> const& session, mf::SurfaceId surface)
{
    wrapped->destroy_surface(session, surface);
}

int msh::ShellWrapper::set_surface_attribute(
    std::shared_ptr<ms::Session> const& session,
    mf::SurfaceId surface_id,
    MirSurfaceAttrib attrib,
    int value)
{
    return wrapped->set_surface_attribute(session, surface_id, attrib, value);
}

int msh::ShellWrapper::get_surface_attribute(
    std::shared_ptr<ms::Session> const& session,
    mf::SurfaceId surface_id,
    MirSurfaceAttrib attrib)
{
    return wrapped->get_surface_attribute(session, surface_id, attrib);
}
