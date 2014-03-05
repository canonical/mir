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
 * Authored By: Nick Dedekind <nick.dedekind@canonical.com>
 */

#include "trust_session.h"
#include "mir/shell/trust_session_creation_parameters.h"
#include "mir/scene/session_container.h"
#include "mir/shell/session.h"
#include "mir/frontend/event_sink.h"

namespace mf = mir::frontend;
namespace ms = mir::scene;
namespace msh = mir::shell;

std::atomic<int> ms::TrustSession::next_session_id{0};

ms::TrustSession::TrustSession(
    std::shared_ptr<msh::Session> const& session,
    msh::TrustSessionCreationParameters const& parameters,
    std::shared_ptr<mf::EventSink> const& sink) :
    trusted_helper(session),
    applications(parameters.applications),
    event_sink(sink),
    started(false),
    current_id(next_id())
{
}

ms::TrustSession::~TrustSession()
{
    TrustSession::stop();
}

mf::SessionId ms::TrustSession::id() const
{
    return current_id;
}

mf::SessionId ms::TrustSession::next_id()
{
    return mf::SessionId(next_session_id.fetch_add(1));
}

bool  ms::TrustSession::get_started() const
{
    return started;
}

std::shared_ptr<msh::Session> ms::TrustSession::get_trusted_helper() const
{
    return trusted_helper;
}

std::shared_ptr<msh::TrustSession> ms::TrustSession::start_for(std::shared_ptr<msh::Session> const& trusted_helper,
                                                                   msh::TrustSessionCreationParameters const& parameters,
                                                                   std::shared_ptr<SessionContainer> const& container,
                                                                   std::shared_ptr<mf::EventSink> const& sink)
{
    TrustSession* impl = new TrustSession(trusted_helper, parameters, sink);
    std::shared_ptr<msh::TrustSession> ptr(impl);

    impl->started = true;
    trusted_helper->set_trust_session(ptr);

    for (pid_t application_pid : impl->applications)
    {
        std::vector<std::shared_ptr<msh::Session>> added_sessions;

        container->for_each(
            [&]
            (std::shared_ptr<msh::Session> const& container_session)
            {
                if (container_session->process_id() == application_pid)
                {
                    impl->add_child_session(container_session);
                    container_session->set_trust_session(ptr);
                    added_sessions.push_back(container_session);
                }
            }
        );
    }

    sink->handle_trust_session_event(impl->id(), mir_trust_session_started);
    return ptr;
}

void ms::TrustSession::stop()
{
    if (!started)
        return;

    trusted_helper->get_children()->for_each(
        [this]
        (std::shared_ptr<msh::Session> const& child_session)
        {
            child_session->set_parent(NULL);
            child_session->set_trust_session(NULL);
        }
    );
    trusted_helper->get_children()->clear();
    trusted_helper->set_trust_session(NULL);

    started = false;
    event_sink->handle_trust_session_event(id(), mir_trust_session_stopped);
}

std::vector<pid_t> ms::TrustSession::get_applications() const
{
  return applications;
}

void ms::TrustSession::add_child_session(std::shared_ptr<msh::Session> const& session)
{
    if (!started)
        return;

    session->set_parent(trusted_helper);
    trusted_helper->get_children()->insert_session(session);
}
