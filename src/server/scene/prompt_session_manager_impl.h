/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
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

#ifndef MIR_SCENE_PROMPT_SESSION_MANAGERIMPL_H_
#define MIR_SCENE_PROMPT_SESSION_MANAGERIMPL_H_

#include "mir/scene/prompt_session_manager.h"
#include "mir_toolkit/common.h"

#include <mutex>
#include <memory>

namespace mir
{
namespace scene
{
class Session;
class SessionContainer;
class PromptSession;
class PromptSessionContainer;
class PromptSessionCreationParameters;
class PromptSessionListener;

class PromptSessionManagerImpl : public scene::PromptSessionManager
{
public:
    explicit PromptSessionManagerImpl(
        std::shared_ptr<SessionContainer> const& app_container,
        std::shared_ptr<PromptSessionListener> const& prompt_session_listener);

    std::shared_ptr<PromptSession> start_prompt_session_for(
        std::shared_ptr<Session> const& session,
        PromptSessionCreationParameters const& params) const override;

    void stop_prompt_session(
        std::shared_ptr<PromptSession> const& prompt_session) const override;

    void suspend_prompt_session(
        std::shared_ptr<PromptSession> const& prompt_session) const override;

    void resume_prompt_session(
        std::shared_ptr<PromptSession> const& prompt_session) const override;

    void add_prompt_provider(
        std::shared_ptr<PromptSession> const& prompt_session,
        std::shared_ptr<Session> const& prompt_provider) const override;

    void remove_session(
        std::shared_ptr<Session> const& session) const override;

    std::shared_ptr<Session> application_for(
        std::shared_ptr<PromptSession> const& prompt_session) const override;

    std::shared_ptr<Session> helper_for(
        std::shared_ptr<PromptSession> const& prompt_session) const override;

    void for_each_provider_in(
        std::shared_ptr<PromptSession> const& prompt_session,
        std::function<void(std::shared_ptr<Session> const& prompt_provider)> const& f) const override;

private:
    std::shared_ptr<PromptSessionContainer> const prompt_session_container;
    std::shared_ptr<PromptSessionListener> const prompt_session_listener;
    std::shared_ptr<SessionContainer> const app_container;

    std::mutex mutable prompt_sessions_mutex;

    void stop_prompt_session_locked(
        std::lock_guard<std::mutex> const&,
        std::shared_ptr<PromptSession> const& prompt_session) const;
};
}
}

#endif // MIR_SCENE_PROMPT_SESSION_MANAGERIMPL_H_
