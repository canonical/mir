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

#include "trust_session_impl.h"
#include "mir/scene/session.h"
#include "mir/scene/trust_session_creation_parameters.h"
#include "mir/scene/trust_session_listener.h"
#include "session_container.h"
#include "trust_session_participants.h"

#include <sstream>
#include <algorithm>

namespace ms = mir::scene;
namespace mf = mir::frontend;

int next_unique_id = 0;

ms::TrustSessionImpl::TrustSessionImpl(
    std::weak_ptr<ms::Session> const& session,
    TrustSessionCreationParameters const&,
    std::shared_ptr<TrustSessionListener> const& trust_session_listener) :
    trusted_helper(session),
    trust_session_listener(trust_session_listener),
    state(mir_trust_session_state_stopped)
{
}

ms::TrustSessionImpl::~TrustSessionImpl()
{
    TrustSessionImpl::stop();
}

MirTrustSessionState ms::TrustSessionImpl::get_state() const
{
    std::lock_guard<decltype(mutex)> lock(mutex);

    return state;
}

std::weak_ptr<ms::Session> ms::TrustSessionImpl::get_trusted_helper() const
{
    std::lock_guard<decltype(mutex)> lock(mutex);

    return trusted_helper;
}

void ms::TrustSessionImpl::start()
{
    std::lock_guard<decltype(mutex)> lock(mutex);

    if (state == mir_trust_session_state_started)
        return;

    state = mir_trust_session_state_started;

    auto helper = trusted_helper.lock();
    if (helper) {
        helper->begin_trust_session();
    }
}

void ms::TrustSessionImpl::stop()
{
    std::lock_guard<decltype(mutex)> lock(mutex);

    if (state == mir_trust_session_state_stopped)
        return;

    state = mir_trust_session_state_stopped;

    auto helper = trusted_helper.lock();
    if (helper) {
        helper->end_trust_session();
    }

    std::vector<mf::Session*> children;
    participants->for_each_participant(
        [&](mf::Session* session)
        {
            children.push_back(session);
        });

    for (auto session : children)
    {
        participants->remove(session);
        // trust_session_listener->trusted_session_ending(*this, std::dynamic_pointer_cast<ms::Session>(session));
    }
}

bool ms::TrustSessionImpl::add_trusted_child(std::shared_ptr<ms::Session> const& session)
{
    std::lock_guard<decltype(mutex)> lock(mutex);

    if (state == mir_trust_session_state_stopped)
        return false;

    if (!participants->insert(session.get()))
        return false;

    trust_session_listener->trusted_session_beginning(*this, session);
    return true;
}

bool ms::TrustSessionImpl::remove_trusted_child(std::shared_ptr<ms::Session> const& session)
{
    std::lock_guard<decltype(mutex)> lock(mutex);

    if (state == mir_trust_session_state_stopped)
        return false;

    if (participants->remove(session.get()))
    {
        trust_session_listener->trusted_session_ending(*this, session);
        return true;
    }
    return false;
}

void ms::TrustSessionImpl::for_each_trusted_child(
    std::function<void(ms::Session*)> f) const
{
    participants->for_each_participant(
        [f](mf::Session* session)
        {
            auto scene_session = dynamic_cast<ms::Session*>(session);
            if (scene_session)
                f(scene_session);
        });
}
