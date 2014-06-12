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

#ifndef MIR_SCENE_PROMPT_SESSION_MANAGER_H_
#define MIR_SCENE_PROMPT_SESSION_MANAGER_H_

#include <sys/types.h>
#include <memory>

namespace mir
{
namespace scene
{
class Session;
class PromptSession;
struct PromptSessionCreationParameters;

class PromptSessionManager
{
public:
    virtual ~PromptSessionManager() = default;

    virtual std::shared_ptr<PromptSession> start_prompt_session_for(std::shared_ptr<Session> const& session,
                                                                    PromptSessionCreationParameters const& params) const = 0;

    virtual void stop_prompt_session(std::shared_ptr<PromptSession> const& prompt_session) const = 0;

    virtual void add_prompt_provider(std::shared_ptr<PromptSession> const& prompt_session,
                                     std::shared_ptr<Session> const& prompt_provider) const = 0;

    virtual void add_prompt_provider_by_pid(std::shared_ptr<PromptSession> const& prompt_session,
                                            pid_t process_id) const = 0;

    virtual void add_expected_session(std::shared_ptr<Session> const& new_session) const = 0;

    virtual void remove_session(std::shared_ptr<Session> const& session) const = 0;

    virtual std::shared_ptr<Session> application_for_prompt_session(std::shared_ptr<PromptSession> const& prompt_session) const = 0;

    virtual std::shared_ptr<Session> helper_for_prompt_session(std::shared_ptr<PromptSession> const& prompt_session) const = 0;

    virtual void for_each_provider_in_prompt_session(std::shared_ptr<PromptSession> const& prompt_session,
                                                     std::function<void(std::shared_ptr<Session> const& prompt_provider)> const& f) const = 0;

protected:
    PromptSessionManager() = default;
    PromptSessionManager(const PromptSessionManager&) = delete;
    PromptSessionManager& operator=(const PromptSessionManager&) = delete;
};

}
}

#endif // MIR_SCENE_PROMPT_SESSION_MANAGER_H_
