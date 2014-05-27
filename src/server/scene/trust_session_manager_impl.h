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

#ifndef MIR_SCENE_TRUST_SESSION_MANAGERIMPL_H_
#define MIR_SCENE_TRUST_SESSION_MANAGERIMPL_H_

#include "mir/scene/trust_session_manager.h"
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

class TrustSessionManagerImpl : public scene::TrustSessionManager
{
public:
    explicit TrustSessionManagerImpl(
        std::shared_ptr<SessionContainer> const& app_container,
        std::shared_ptr<TrustSessionListener> const& trust_session_listener);

    ~TrustSessionManagerImpl() noexcept;

    std::shared_ptr<TrustSession> start_trust_session_for(
        std::shared_ptr<Session> const& session,
        TrustSessionCreationParameters const& params) const override;

    void stop_trust_session(
        std::shared_ptr<TrustSession> const& trust_session) const override;

    void add_participant(
        std::shared_ptr<TrustSession> const& trust_session,
        std::shared_ptr<Session> const& session) const override;

    void add_participant_by_pid(
        std::shared_ptr<TrustSession> const& trust_session,
        pid_t process_id) const override;

    void add_expected_session(
        std::shared_ptr<Session> const& new_session) const override;

    void remove_session(
        std::shared_ptr<Session> const& session) const override;

    void for_each_participant_in_trust_session(
        std::shared_ptr<TrustSession> const& trust_session,
        std::function<void(std::shared_ptr<Session> const& participant)> const& f) const override;

private:
    std::shared_ptr<TrustSessionContainer> const trust_session_container;
    std::shared_ptr<TrustSessionListener> const trust_session_listener;
    std::shared_ptr<SessionContainer> const app_container;

    std::mutex mutable trust_sessions_mutex;

    void add_participant_by_pid_locked(
        std::lock_guard<std::mutex> const&,
        std::shared_ptr<TrustSession> const& trust_session,
        pid_t process_id) const;

    void stop_trust_session_locked(
        std::lock_guard<std::mutex> const&,
        std::shared_ptr<TrustSession> const& trust_session) const;
};
}
}

#endif // MIR_SCENE_TRUST_SESSION_MANAGERIMPL_H_
