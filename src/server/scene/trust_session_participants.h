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
class TrustSessionContainer;

class TrustSessionParticipants
{
public:
    TrustSessionParticipants(frontend::TrustSession* trust_session, std::shared_ptr<TrustSessionContainer> const& container);
    virtual ~TrustSessionParticipants() = default;

    bool insert(std::weak_ptr<frontend::Session> const& session);
    bool remove(std::weak_ptr<frontend::Session> const& session);
    void clear();

    bool contains(std::weak_ptr<frontend::Session> const& session) const;

    void for_each_participant(std::function<void(std::weak_ptr<frontend::Session> const&)> f) const;

private:
    frontend::TrustSession* trust_session;
    std::shared_ptr<TrustSessionContainer> container;
};

}
}

#endif // MIR_SCENE_TRUST_SESSION_PARTICIPANTS_H_
