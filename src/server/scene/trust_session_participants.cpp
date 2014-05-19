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

 #include "trust_session_participants.h"
 #include "trust_session_container.h"

namespace ms = mir::scene;
namespace mf = mir::frontend;

ms::TrustSessionParticipants::TrustSessionParticipants(
    frontend::TrustSession* trust_session,
    std::shared_ptr<TrustSessionContainer> const& container) :
    trust_session(trust_session),
    container(container)
{
}

bool ms::TrustSessionParticipants::insert(std::weak_ptr<mf::Session> const& session)
{
    return container->insert_participant(trust_session, session, true);
}

bool ms::TrustSessionParticipants::remove(std::weak_ptr<mf::Session> const& session)
{
    return container->remove_participant(trust_session, session);
}

bool ms::TrustSessionParticipants::contains(std::weak_ptr<mf::Session> const& session) const
{
    bool found = false;
    for_each_participant([&](std::weak_ptr<mf::Session> const& participant)
        {
            if (session.lock() == participant.lock())
            {
                found |= true;
            }
        });
    return found;
}

void ms::TrustSessionParticipants::for_each_participant(std::function<void(std::weak_ptr<mf::Session> const&)> f) const
{
    container->for_each_participant_for_trust_session(trust_session,
        [&](std::weak_ptr<mf::Session> const& session, bool is_child)
        {
            if (is_child)
                f(session);
        });
}
