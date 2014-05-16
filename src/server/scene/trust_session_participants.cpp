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
 #include "trust_session_participant_container.h"

namespace ms = mir::scene;
namespace mf = mir::frontend;

ms::TrustSessionParticipants::TrustSessionParticipants(
    std::weak_ptr<mf::TrustSession> const& trust_session,
    std::shared_ptr<TrustSessionParticipantContainer> const& container) :
    trust_session(trust_session),
    container(container)
{
}

bool ms::TrustSessionParticipants::insert(mf::Session* session)
{
    if (auto locked_session = trust_session.lock())
    {
        return container->insert_participant(locked_session, session, true);
    }
    return false;
}

bool ms::TrustSessionParticipants::remove(mf::Session* session)
{
    if (auto locked_session = trust_session.lock())
    {
        return container->remove_participant(locked_session, session);
    }
    return false;
}

bool ms::TrustSessionParticipants::contains(mf::Session* session) const
{
    bool found = false;
    for_each_participant([&](mf::Session* participant)
        {
            if (session == participant)
            {
                found |= true;
            }
        });
    return found;
}

void ms::TrustSessionParticipants::for_each_participant(std::function<void(mf::Session*)> f) const
{
    if (auto locked_session = trust_session.lock())
    {
        container->for_each_participant_for_trust_session(locked_session,
            [f](mf::Session* session, bool is_child)
            {
                if (is_child)
                    f(session);
            });
    }
}
