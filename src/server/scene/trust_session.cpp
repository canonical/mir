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
#include "mir/shell/session.h"
#include "session_container.h"

namespace ms = mir::scene;
namespace msh = mir::shell;

ms::TrustSession::TrustSession(
    std::weak_ptr<msh::Session> const& session,
    msh::TrustSessionCreationParameters const& parameters) :
    trusted_helper(session),
    applications(parameters.applications),
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

std::weak_ptr<msh::Session> ms::TrustSession::get_trusted_helper() const
{
    return trusted_helper;
}

void ms::TrustSession::start()
{
    if (state == mir_trust_session_state_started)
        return;

    state = mir_trust_session_state_started;
}

void ms::TrustSession::stop()
{
    if (state == mir_trust_session_state_stopped)
        return;

    state = mir_trust_session_state_stopped;

    auto helper = trusted_helper.lock();
    if (helper)
        helper->end_trust_session();
}

std::vector<pid_t> ms::TrustSession::get_applications() const
{
  return applications;
}

void ms::TrustSession::add_trusted_child(std::shared_ptr<msh::Session> const& session)
{
    if (state == mir_trust_session_state_stopped)
        return;

    auto helper = trusted_helper.lock();
    if (helper)
        helper->add_trusted_child(session);
}

void ms::TrustSession::remove_trusted_child(std::shared_ptr<shell::Session> const& session)
{
    if (state == mir_trust_session_state_stopped)
        return;

    auto helper = trusted_helper.lock();
    if (helper)
        helper->remove_trusted_child(session);
}
