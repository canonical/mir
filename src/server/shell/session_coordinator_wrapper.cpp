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


std::shared_ptr<ms::Session> msh::SessionCoordinatorWrapper::open_session(
    pid_t client_pid,
    std::string const& name,
    std::shared_ptr<mf::EventSink> const& sink)
{
    return wrapped->open_session(client_pid, name, sink);
}

void msh::SessionCoordinatorWrapper::close_session(
    std::shared_ptr<ms::Session> const& session)
{
    wrapped->close_session(session);
}

std::shared_ptr<ms::Session> msh::SessionCoordinatorWrapper::successor_of(
    std::shared_ptr<ms::Session> const& session) const
{
    return wrapped->successor_of(session);
}

void msh::SessionCoordinatorWrapper::set_focus_to(
    std::shared_ptr<scene::Session> const& focus)
{
    wrapped->set_focus_to(focus);
}

void msh::SessionCoordinatorWrapper::unset_focus()
{
    wrapped->unset_focus();
}

std::shared_ptr<ms::PromptSession> msh::SessionCoordinatorWrapper::start_prompt_session_for(
    std::shared_ptr<ms::Session> const& session,
    scene::PromptSessionCreationParameters const& params)
{
    return wrapped->start_prompt_session_for(session, params);
}

void msh::SessionCoordinatorWrapper::add_prompt_provider_for(
    std::shared_ptr<ms::PromptSession> const& prompt_session,
    std::shared_ptr<ms::Session> const& session)
{
    wrapped->add_prompt_provider_for(prompt_session, session);
}

void msh::SessionCoordinatorWrapper::stop_prompt_session(std::shared_ptr<ms::PromptSession> const& prompt_session)
{
    wrapped->stop_prompt_session(prompt_session);
}
