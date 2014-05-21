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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef MIR_SCENE_TRUST_SESSION_MANAGER_H_
#define MIR_SCENE_TRUST_SESSION_MANAGER_H_

#include "mir_toolkit/common.h"

#include <mutex>
#include <memory>

namespace mir
{
namespace scene
{
class Session;
class SessionContainer;
class TrustSession;
class TrustSessionContainer;
class TrustSessionCreationParameters;
class TrustSessionListener;

class TrustSessionManager
{
public:
    explicit TrustSessionManager(
        std::shared_ptr<TrustSessionListener> const& trust_session_listener);

    ~TrustSessionManager() noexcept;

    std::shared_ptr<TrustSession> start_trust_session_for(
        std::shared_ptr<Session> const& session,
        TrustSessionCreationParameters const& params,
        SessionContainer const& existing_session) const;

    void add_expected_session(std::shared_ptr<Session> const& new_session) const;

    MirTrustSessionAddTrustResult add_trusted_process_for(
        std::shared_ptr<TrustSession> const& trust_session,
        pid_t process_id,
        SessionContainer const& existing_session) const;

    void stop_trust_session(std::shared_ptr<TrustSession> const& trust_session) const;
    void remove_session(std::shared_ptr<Session> const& session) const;

private:
    std::shared_ptr<TrustSessionContainer> const trust_session_container;
    std::shared_ptr<TrustSessionListener> const trust_session_listener;

    std::mutex mutable trust_sessions_mutex;

    MirTrustSessionAddTrustResult add_trusted_process_for_locked(
        std::lock_guard<std::mutex> const&,
        std::shared_ptr<TrustSession> const& trust_session,
        pid_t process_id,
        SessionContainer const& existing_session) const;

    void stop_trust_session_locked(
        std::lock_guard<std::mutex> const&,
        std::shared_ptr<TrustSession> const& trust_session) const;
};
}
}

#endif // MIR_SCENE_TRUST_SESSION_MANAGER_H_
