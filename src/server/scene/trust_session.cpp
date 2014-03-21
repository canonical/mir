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
    std::unique_lock<std::mutex> lock(mutex);

    return state;
}

std::weak_ptr<msh::Session> ms::TrustSession::get_trusted_helper() const
{
    std::unique_lock<std::mutex> lock(mutex);

    return trusted_helper;
}

void ms::TrustSession::start()
{
    std::unique_lock<std::mutex> lock(mutex);

    if (state == mir_trust_session_state_started)
        return;

    state = mir_trust_session_state_started;
}

void ms::TrustSession::stop()
{
    {
        std::unique_lock<std::mutex> lock(mutex);

        if (state == mir_trust_session_state_stopped)
            return;

        state = mir_trust_session_state_stopped;

        auto helper = trusted_helper.lock();
        if (helper)
            helper->end_trust_session();
    }

    for_each_trusted_child(
        [](std::shared_ptr<msh::Session> const& child_session)
        {
            child_session->end_trust_session();
            return true;
        },
        false
    );
    clear_trusted_children();
}

std::vector<pid_t> ms::TrustSession::get_applications() const
{
    return applications;
}

void ms::TrustSession::add_trusted_child(std::shared_ptr<msh::Session> const& session)
{
    std::unique_lock<std::mutex> lock(mutex);

    if (state == mir_trust_session_state_stopped)
        return;

    trusted_children.push_back(session);
}

void ms::TrustSession::remove_trusted_child(std::shared_ptr<msh::Session> const& session)
{
    std::unique_lock<std::mutex> lock(mutex);

    if (state == mir_trust_session_state_stopped)
        return;

    for (auto it = trusted_children.begin(); it != trusted_children.end(); ++it)
    {
        auto trusted_child = (*it).lock();
        if (trusted_child == session) {

            trusted_child->end_trust_session();

            trusted_children.erase(it);
            break;
        }
    }
}

void ms::TrustSession::for_each_trusted_child(
    std::function<bool(std::shared_ptr<msh::Session> const&)> f,
    bool reverse) const
{
    std::unique_lock<std::mutex> lk(mutex);

    if (reverse)
    {
        for (auto rit = trusted_children.rbegin(); rit != trusted_children.rend(); ++rit)
        {
            auto trusted_child = (*rit).lock();
            if (trusted_child && !f(trusted_child))
                break;
        }
    }
    else
    {
        for (auto it = trusted_children.begin(); it != trusted_children.end(); ++it)
        {
            auto trusted_child = (*it).lock();
            if (trusted_child && !f(trusted_child))
                break;
        }
    }
}

void ms::TrustSession::clear_trusted_children()
{
    std::unique_lock<std::mutex> lock(mutex);
    trusted_children.clear();
}
