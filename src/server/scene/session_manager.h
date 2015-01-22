/*
 * Copyright © 2012-2014 Canonical Ltd.
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
 * Authored by: Thomas Voss <thomas.voss@canonical.com>
 */

#ifndef MIR_SCENE_APPLICATION_MANAGER_H_
#define MIR_SCENE_APPLICATION_MANAGER_H_

#include "mir/scene/session_coordinator.h"

#include <memory>
#include <vector>

namespace mir
{
namespace scene
{
class SessionContainer;
class SessionEventSink;
class SessionListener;
class SnapshotStrategy;
class SurfaceCoordinator;
class PromptSessionManager;


class SessionManager : public SessionCoordinator
{
public:
    explicit SessionManager(std::shared_ptr<SurfaceCoordinator> const& surface_coordinator,
                            std::shared_ptr<SessionContainer> const& app_container,
                            std::shared_ptr<SnapshotStrategy> const& snapshot_strategy,
                            std::shared_ptr<SessionEventSink> const& session_event_sink,
                            std::shared_ptr<SessionListener> const& session_listener,
                            std::shared_ptr<PromptSessionManager> const& prompt_session_manager);
    virtual ~SessionManager() noexcept;

    std::shared_ptr<Session> open_session(
        pid_t client_pid,
        std::string const& name,
        std::shared_ptr<frontend::EventSink> const& sink) override;

    void close_session(std::shared_ptr<Session> const& session) override;

    std::shared_ptr<Session> successor_of(std::shared_ptr<Session> const&) const override;

    void set_focus_to(std::shared_ptr<Session> const& focus) override;
    void unset_focus() override;

    std::shared_ptr<PromptSession> start_prompt_session_for(std::shared_ptr<Session> const& session,
                                                  PromptSessionCreationParameters const& params) override;
    void add_prompt_provider_for(std::shared_ptr<PromptSession> const& prompt_session,
                                 std::shared_ptr<Session> const& session) override;
    void stop_prompt_session(std::shared_ptr<PromptSession> const& prompt_session) override;

protected:
    SessionManager(const SessionManager&) = delete;
    SessionManager& operator=(const SessionManager&) = delete;

private:
    std::shared_ptr<SurfaceCoordinator> const surface_coordinator;
    std::shared_ptr<SessionContainer> const app_container;
    std::shared_ptr<SnapshotStrategy> const snapshot_strategy;
    std::shared_ptr<SessionEventSink> const session_event_sink;
    std::shared_ptr<SessionListener> const session_listener;
    std::shared_ptr<PromptSessionManager> const prompt_session_manager;
};

}
}

#endif // MIR_SCENE_APPLICATION_MANAGER_H_
