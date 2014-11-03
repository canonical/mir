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

#include <mutex>
#include <memory>
#include <vector>

namespace mir
{

namespace shell
{
class FocusSetter;
}

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
                            std::shared_ptr<shell::FocusSetter> const& focus_setter,
                            std::shared_ptr<SnapshotStrategy> const& snapshot_strategy,
                            std::shared_ptr<SessionEventSink> const& session_event_sink,
                            std::shared_ptr<SessionListener> const& session_listener,
                            std::shared_ptr<PromptSessionManager> const& prompt_session_manager);
    virtual ~SessionManager() noexcept;

    virtual std::shared_ptr<frontend::Session> open_session(
        pid_t client_pid,
        std::string const& name,
        std::shared_ptr<frontend::EventSink> const& sink) override;

    virtual void close_session(std::shared_ptr<frontend::Session> const& session) override;

    void focus_next() override;
    std::weak_ptr<Session> focussed_application() const override;
    void set_focus_to(std::shared_ptr<Session> const& focus) override;

    void handle_surface_created(std::shared_ptr<frontend::Session> const& session) override;

    std::shared_ptr<frontend::PromptSession> start_prompt_session_for(std::shared_ptr<frontend::Session> const& session,
                                                  PromptSessionCreationParameters const& params) override;
    void add_prompt_provider_for(std::shared_ptr<frontend::PromptSession> const& prompt_session,
                                 std::shared_ptr<frontend::Session> const& session) override;
    void stop_prompt_session(std::shared_ptr<frontend::PromptSession> const& prompt_session) override;

protected:
    SessionManager(const SessionManager&) = delete;
    SessionManager& operator=(const SessionManager&) = delete;

private:
    std::shared_ptr<SurfaceCoordinator> const surface_coordinator;
    std::shared_ptr<SessionContainer> const app_container;
    std::shared_ptr<shell::FocusSetter> const focus_setter;
    std::shared_ptr<SnapshotStrategy> const snapshot_strategy;
    std::shared_ptr<SessionEventSink> const session_event_sink;
    std::shared_ptr<SessionListener> const session_listener;
    std::shared_ptr<PromptSessionManager> const prompt_session_manager;

    std::mutex mutex;
    std::weak_ptr<Session> focus_application;

    void set_focus_to_locked(std::unique_lock<std::mutex> const& lock, std::shared_ptr<Session> const& next_focus);
};

}
}

#endif // MIR_SCENE_APPLICATION_MANAGER_H_
