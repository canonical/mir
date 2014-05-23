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

#ifndef MIR_SCENE_NULL_TRUST_SESSION_MANAGER_H_
#define MIR_SCENE_NULL_TRUST_SESSION_MANAGER_H_

#include "mir/scene/trust_session_manager.h"

namespace mir
{
namespace test
{
namespace doubles
{

class NullTrustSessionManager: public scene::TrustSessionManager
{
public:
    std::shared_ptr<scene::TrustSession> start_trust_session_for(std::shared_ptr<scene::Session> const&,
                                                          scene::TrustSessionCreationParameters const&) const
    {
      return std::shared_ptr<scene::TrustSession>();
    }

    void stop_trust_session(std::shared_ptr<scene::TrustSession> const&) const
    {
    }

    void add_participant(std::shared_ptr<scene::TrustSession> const&,
                         std::shared_ptr<scene::Session> const&) const
    {
    }

    void add_participant_by_pid(std::shared_ptr<scene::TrustSession> const&,
                                pid_t) const
    {
    }

    void add_expected_session(std::shared_ptr<scene::Session> const&) const
    {
    }

    void remove_session(std::shared_ptr<scene::Session> const&) const
    {
    }

    void for_each_participant_in_trust_session(std::shared_ptr<scene::TrustSession> const&,
                                               std::function<void(std::shared_ptr<scene::Session> const& participant)> const&) const
    {
    }
};

}
}
}

#endif // MIR_SCENE_NULL_TRUST_SESSION_MANAGER_H_
