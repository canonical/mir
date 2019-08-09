/*
 * Copyright © 2014 Canonical Ltd.
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
 *
 * Authored By: Alan Griffiths <alan@octopull.co.uk>
 */

#include "shell_wrapper.h"

namespace mf = mir::frontend;
namespace ms = mir::scene;

std::shared_ptr<mf::MirClientSession> mf::ShellWrapper::open_session(
    pid_t client_pid,
    std::string const& name,
    std::shared_ptr<EventSink> const& sink)
{
    return wrapped->open_session(client_pid, name,sink);
}

void mf::ShellWrapper::close_session(std::shared_ptr<MirClientSession> const& session)
{
    wrapped->close_session(session);
}

auto mf::ShellWrapper::scene_session_for(
    std::shared_ptr<MirClientSession> const& session) -> std::shared_ptr<scene::Session>
{
    return wrapped->scene_session_for(session);
}

std::shared_ptr<mf::PromptSession> mf::ShellWrapper::start_prompt_session_for(
    std::shared_ptr<scene::Session> const& session,
    scene::PromptSessionCreationParameters const& params)
{
    return wrapped->start_prompt_session_for(session, params);
}

void mf::ShellWrapper::add_prompt_provider_for(
    std::shared_ptr<PromptSession> const& prompt_session,
    std::shared_ptr<scene::Session> const& session)
{
    wrapped->add_prompt_provider_for(prompt_session, session);
}

void mf::ShellWrapper::stop_prompt_session(
    std::shared_ptr<PromptSession> const& prompt_session)
{
    wrapped->stop_prompt_session(prompt_session);
}

mf::SurfaceId mf::ShellWrapper::create_surface(
    std::shared_ptr<MirClientSession> const& session,
    scene::SurfaceCreationParameters const& params,
    std::shared_ptr<EventSink> const& sink)
{
    return wrapped->create_surface(session, params, sink);
}

void mf::ShellWrapper::modify_surface(std::shared_ptr<MirClientSession> const& session, SurfaceId surface, shell::SurfaceSpecification const& modifications)
{
    wrapped->modify_surface(session, surface, modifications);
}

void mf::ShellWrapper::destroy_surface(std::shared_ptr<MirClientSession> const& session, SurfaceId surface)
{
    wrapped->destroy_surface(session, surface);
}

std::string mf::ShellWrapper::persistent_id_for(std::shared_ptr<MirClientSession> const& session, mf::SurfaceId surface)
{
    return wrapped->persistent_id_for(session, surface);
}

std::shared_ptr<ms::Surface> mf::ShellWrapper::surface_for_id(std::string const& serialised_id)
{
    return wrapped->surface_for_id(serialised_id);
}

int mf::ShellWrapper::set_surface_attribute(
    std::shared_ptr<MirClientSession> const& session,
    SurfaceId surface_id,
    MirWindowAttrib attrib,
    int value)
{
    return wrapped->set_surface_attribute(session, surface_id, attrib, value);
}

int mf::ShellWrapper::get_surface_attribute(
    std::shared_ptr<MirClientSession> const& session,
    SurfaceId surface_id,
    MirWindowAttrib attrib)
{
    return wrapped->get_surface_attribute(session, surface_id, attrib);
}

void mf::ShellWrapper::request_operation(
    std::shared_ptr<MirClientSession> const& session,
    SurfaceId surface_id,
    uint64_t timestamp,
    UserRequest request,
    optional_value <uint32_t> hint)
{
    wrapped->request_operation(session, surface_id, timestamp, request, hint);
}
