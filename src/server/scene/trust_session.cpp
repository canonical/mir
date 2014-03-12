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

ms::TrustSession::TrustSession(
    std::shared_ptr<msh::Session> const& session,
    msh::TrustSessionCreationParameters const& parameters,
    std::shared_ptr<mf::EventSink> const& sink) :
    trusted_helper(session),
    applications(parameters.applications),
    event_sink(sink),
    state(mir_trust_session_state_stopped)
{
}

ms::TrustSession::~TrustSession()
{
    TrustSession::stop();
}

MirTrustSessionState  ms::TrustSession::get_state() const
{
    return state;
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
    auto const trust_session = std::make_shared<TrustSession>(trusted_helper, parameters, sink);

    trust_session->state = mir_trust_session_state_started;
    trusted_helper->set_trust_session(trust_session);

    for (pid_t application_pid : trust_session->applications)
    {
        std::vector<std::shared_ptr<msh::Session>> added_sessions;

        container->for_each(
            [&](std::shared_ptr<msh::Session> const& container_session)
            {
                if (container_session->process_id() == application_pid)
                {
                    trust_session->add_child_session(container_session);
                    container_session->set_trust_session(trust_session);
                    added_sessions.push_back(container_session);
                }
            }
        );
    }

    MirEvent start_session;
    start_session.type = mir_event_type_trust_session_state_change;
    start_session.trust_session.new_state = trust_session->state;
    sink->handle_event(start_session);

    return trust_session;
}

void ms::TrustSession::stop()
{
    if (state == mir_trust_session_state_stopped)
        return;

    trusted_helper->get_children()->for_each(
        [this](std::shared_ptr<msh::Session> const& child_session)
        {
            child_session->set_parent(NULL);
            child_session->set_trust_session(NULL);
        }
    );
    trusted_helper->get_children()->clear();
    trusted_helper->set_trust_session(NULL);

    state = mir_trust_session_state_stopped;

    MirEvent start_session;
    start_session.type = mir_event_type_trust_session_state_change;
    start_session.trust_session.new_state = state;
    event_sink->handle_event(start_session);
}

std::vector<pid_t> ms::TrustSession::get_applications() const
{
  return applications;
}

void ms::TrustSession::add_child_session(std::shared_ptr<msh::Session> const& session)
{
    if (state == mir_trust_session_state_stopped)
        return;

    session->set_parent(trusted_helper);
    trusted_helper->get_children()->insert_session(session);
}
