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

 #include "trust_session_trusted_participants.h"
 #include "trust_session_container.h"
 #include "mir/scene/trust_session.h"
 #include "mir/scene/session.h"

namespace ms = mir::scene;
namespace mf = mir::frontend;

ms::TrustSessionTrustedParticipants::TrustSessionTrustedParticipants(
    ms::TrustSession* trust_session,
    std::shared_ptr<TrustSessionContainer> const& container) :
    trust_session(trust_session),
    container(container)
{
}

bool ms::TrustSessionTrustedParticipants::insert(std::weak_ptr<Session> const& session)
{
    return container->insert_participant(trust_session, session, TrustSessionContainer::TrustedSession);
}

bool ms::TrustSessionTrustedParticipants::remove(std::weak_ptr<Session> const& session)
{
    return container->remove_participant(trust_session, session, TrustSessionContainer::TrustedSession);
}

bool ms::TrustSessionTrustedParticipants::contains(std::weak_ptr<Session> const& session) const
{
    bool found = false;
    for_each_trusted_participant([&](std::weak_ptr<Session> const& participant)
        {
            if (session.lock() == participant.lock())
            {
                found |= true;
            }
        });
    return found;
}

void ms::TrustSessionTrustedParticipants::for_each_trusted_participant(std::function<void(std::weak_ptr<Session> const&)> f) const
{
    container->for_each_participant_in_trust_session(trust_session,
        [&](std::weak_ptr<Session> const& session, ms::TrustSessionContainer::TrustType trust_type)
        {
            if (trust_type == TrustSessionContainer::TrustedSession)
                f(session);
        });
}
