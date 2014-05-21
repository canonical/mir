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
#include "trust_session_container.h"
#include "trust_session_trusted_participants.h"

#include <sstream>
#include <algorithm>

namespace ms = mir::scene;
namespace mf = mir::frontend;

int next_unique_id = 0;

ms::TrustSessionImpl::TrustSessionImpl(
    std::weak_ptr<ms::Session> const& session,
    TrustSessionCreationParameters const&,
    std::shared_ptr<TrustSessionListener> const& trust_session_listener,
    std::shared_ptr<TrustSessionContainer> const& container) :
    trusted_helper(session),
    trust_session_listener(trust_session_listener),
    participants(std::make_shared<TrustSessionTrustedParticipants>(this, container)),
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

    std::vector<std::shared_ptr<Session>> children;
    participants->for_each_trusted_participant([&](std::weak_ptr<Session> const& session)
        {
            if (auto locked_session = session.lock())
                children.push_back(locked_session);
        });

    for (auto session : children)
    {
        participants->remove(session);
        trust_session_listener->trusted_session_ending(*this, std::dynamic_pointer_cast<ms::Session>(session));
    }
}

void ms::TrustSessionImpl::add_trusted_participant(std::shared_ptr<ms::Session> const& session)
{
    std::lock_guard<decltype(mutex)> lock(mutex);

    if (state == mir_trust_session_state_stopped)
        return;

    if (participants->insert(session))
    {
        trust_session_listener->trusted_session_beginning(*this, session);
    }
}

void ms::TrustSessionImpl::remove_trusted_participant(std::shared_ptr<ms::Session> const& session)
{
    std::lock_guard<decltype(mutex)> lock(mutex);

    if (state == mir_trust_session_state_stopped)
        return;

    if (participants->remove(session))
    {
        trust_session_listener->trusted_session_ending(*this, session);
    }
}

void ms::TrustSessionImpl::for_each_trusted_participant(
    std::function<void(std::shared_ptr<Session> const&)> f) const
{
    participants->for_each_trusted_participant([f](std::weak_ptr<Session> const& session)
        {
            if (auto locked_scene_session = std::dynamic_pointer_cast<ms::Session>(session.lock()))
                f(locked_scene_session);
        });
}
