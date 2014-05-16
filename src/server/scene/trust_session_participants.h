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

#ifndef MIR_SCENE_TRUST_SESSION_PARTICIPANTS_H_
#define MIR_SCENE_TRUST_SESSION_PARTICIPANTS_H_

#include <memory>

namespace mir
{
namespace frontend
{
class TrustSession;
class Session;
}

namespace scene
{
class TrustSessionParticipantContainer;

class TrustSessionParticipants
{
public:
    TrustSessionParticipants(std::weak_ptr<frontend::TrustSession> const& trust_session, std::shared_ptr<TrustSessionParticipantContainer> const& container);

    bool insert(frontend::Session* session);
    bool remove(frontend::Session* session);

    bool contains(frontend::Session* session) const;

    void for_each_participant(std::function<void(frontend::Session*)> f) const;

private:
    std::weak_ptr<frontend::TrustSession> trust_session;
    std::shared_ptr<TrustSessionParticipantContainer> container;
};

}
}

#endif // MIR_SCENE_TRUST_SESSION_PARTICIPANTS_H_
